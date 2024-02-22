#include "Processor.h"

#include "Logging.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <arte/Image.h>
#include <arte/ImageConvolution.h>

#include <math/Box.h>
#include <math/Homogeneous.h>

#include <snac-renderer-V2/Material.h>
#include <snac-renderer-V2/files/BinaryArchive.h>
#include <snac-renderer-V2/files/Flags.h>

#include <fmt/ostream.h>

#include <fstream>
#include <iostream>
#include <span>


namespace ad::renderer {

namespace {

    [[maybe_unused]] bool hasTrianglesOnly(aiMesh * aMesh)
    {
        //TODO Ad 2023/07/21: Understand what is this NGON encoding flag for.
        return (aMesh->mPrimitiveTypes = (aiPrimitiveType_TRIANGLE | aiPrimitiveType_NGONEncodingFlag));
    }

    math::Matrix<4, 3, float> extractAffinePart(const aiNode * aNode)
    {
        auto m = aNode->mTransformation;
        assert(m.d1 == 0 && m.d2 == 0 && m.d3 == 0 && m.d4 == 1);
        return math::Matrix<4, 3, float>{
            m.a1, m.b1, m.c1,
            m.a2, m.b2, m.c2,
            m.a3, m.b3, m.c3,
            m.a4, m.b4, m.c4,
        };
    }

    math::Position<3, float> toPosition(aiVector3D aVec)
    {
        return {aVec.x, aVec.y, aVec.z};
    }

    math::Box<float> extractAabb(const aiMesh * aMesh)
    {
        aiAABB aiBox = aMesh->mAABB;
        return{
            .mPosition = toPosition(aiBox.mMin),
            .mDimension = (toPosition(aiBox.mMax) - toPosition(aiBox.mMin)).as<math::Size>(),
        };
    }

    
    struct FileWriter
    {
        FileWriter(std::filesystem::path aDestinationFile) :
            mArchive{
                .mOut = std::ofstream{aDestinationFile, std::ios::binary},
                .mParentPath = aDestinationFile.parent_path(),
            }
        {
            unsigned int version = 1;
            mArchive.write(version);
        }

        void write(const aiNode * aNode)
        {
            mArchive.write(std::string{aNode->mName.C_Str()});
            mArchive.write(aNode->mNumMeshes);
            mArchive.write(aNode->mNumChildren);
            mArchive.write(extractAffinePart(aNode));
        }

        void write(const aiMesh * aMesh)
        {
            mArchive.write(std::string{aMesh->mName.C_Str()});

            //TODO Ad 2023/07/27: It could be better to dump only the materials actually references by meshes
            // (requires a step to re-index without the unused materials)
            // Material index
            mArchive.write(aMesh->mMaterialIndex);

            aiVector3D * tangents = aMesh->mTangents;
            // positions and normals are always required
            unsigned int vertexAttributesFlags = gVertexPosition | gVertexNormal;
            // Note: For the moment, since the structure of the loader if too rigid
            // the tangents are mandatory (the vertexAttributesFlags is an illusion)
            // TODO Ad 2024/02/21: #assetprocessor Can we handle "dynamic" list of vertex attributes?
            // (e.g., making tangents optional)
            assert(tangents);
            if(tangents) vertexAttributesFlags |= gVertexTangent;
            mArchive.write(vertexAttributesFlags);

            // Vertices
            mArchive.write(aMesh->mNumVertices);
            mArchive.write(std::span{aMesh->mVertices, aMesh->mNumVertices});

            assert(aMesh->mNormals != nullptr);
            mArchive.write(std::span{aMesh->mNormals, aMesh->mNumVertices});

            if(tangents)
            {
                mArchive.write(std::span{tangents, aMesh->mNumVertices});
            }

            mArchive.write(aMesh->GetNumColorChannels());
            for (unsigned int colorIdx = 0; colorIdx != aMesh->GetNumColorChannels(); ++colorIdx)
            {
                mArchive.write(std::span{aMesh->mColors[colorIdx], aMesh->mNumVertices});
            }

            mArchive.write(aMesh->GetNumUVChannels());
            for (unsigned int uvIdx = 0; uvIdx != aMesh->GetNumUVChannels(); ++uvIdx)
            {
                // Only support bidimensionnal texture sampling atm.
                // Sadly, some models have a 3 here, even if they actually use only 2 (e.g. teapot.obj)
                //assert(aMesh->mNumUVComponents[uvIdx] == 2);
                // TODO Interleave the odd channel with their preceding even channel
                // (i.e. better usage of the 4 components of each vertex attribute)
                for(unsigned int vertexIdx = 0; vertexIdx != aMesh->mNumVertices; ++vertexIdx)
                {
                    mArchive.write(std::span{&(aMesh->mTextureCoords[uvIdx][vertexIdx].x), 2});
                    // Even the assertion below does not hold true for teapot.obj
                    //assert(aMesh->mTextureCoords[uvIdx][vertexIdx].z == 0);
                }
            }

            // Faces
            mArchive.write(aMesh->mNumFaces);
            for(std::size_t faceIdx = 0; faceIdx != aMesh->mNumFaces; ++faceIdx)
            {
                const aiFace & face = aMesh->mFaces[faceIdx];
                mArchive.write(std::span{face.mIndices, 3});
            }
        }

        void write(const std::vector<std::string> & aStrings)
        {
            mArchive.write((unsigned int)aStrings.size());
            for(const auto & string : aStrings)
            {
                mArchive.write(string);
            }
        }

        template <class T_element>
        void writeRaw(std::span<T_element> aData)
        {
            mArchive.write((unsigned int)aData.size());
            mArchive.write(aData);
        }

        // TODO this feels out of place, this file writer being inteded to be high level
        template <class T_data>
        void forward(const T_data & aData)
        {
            mArchive.write(aData);
        }

        BinaryOutArchive mArchive;
    };
    

    /// @brief Return type on "visiting" a node (i.e. recurseNode())
    struct NodeResult
    {
        math::Box<float> mAabb;
        unsigned int mVerticesCount = 0;
        unsigned int mIndicesCount = 0;
    };

    
    BinaryOutArchive::Position_t reservePreamble(FileWriter & aWriter)
    {
        BinaryOutArchive::Position_t preamblePosition = aWriter.mArchive.getPosition();
        // Vertices count
        aWriter.forward(0u);
        // Indices count
        aWriter.forward(0u);
        return preamblePosition;
    }


    // TODO #binarypreamble This preamble approach assumes that all vertices have the same set of attributes
    // (i.e. position, normal, uv, ...)
    void completePreamble(FileWriter & aWriter,
                          BinaryOutArchive::Position_t aPreamblePosition,
                          const NodeResult & aResult)
    {
        auto initialPosition = aWriter.mArchive.getPosition();
        aWriter.mArchive.mOut.seekp(aPreamblePosition);
        aWriter.forward(aResult.mVerticesCount);
        aWriter.forward(aResult.mIndicesCount);
        aWriter.mArchive.mOut.seekp(initialPosition);
    }


    // Current binary format as of 2023/07/25:
    //  vertices count
    //  indices count
    //  Node:
    //    node.name
    //    node.numMeshes
    //    node.numChildren
    //    node.transformation
    //    each node.mesh:
    //      mesh.name
    //      mesh.materialIndex
    //      mesh.vertexAttributesFlags
    //      mesh.numVertices
    //      [mesh.vertices(i.e. positions, 3 floats per vertex)]
    //      [mesh.normals(3 floats per vertex)]
    //      <if mesh.hasTangents>: [mesh.tangents(3 floats per vertex)]
    //      mesh.numColorChannels
    //      each mesh.ColorChannel:
    //        [ColorChannel.color(4 floats per vertex)]
    //      mesh.numUVChannels
    //      each mesh.UVChannel:
    //        [UVChannel.coordinates(2 floats per vertex)]
    //      mesh.numFaces
    //      [mesh.faces(i.e. 3 unsigned int per face)]
    //      mesh.boundingBox (AABB, as a math::Box<float>)
    //    bounding box union of all meshes (AABB, as a math::Box<float>, even if there are no meshes)
    //    each node.child:
    //      // recurse Node:
    //    node.boundingBox (AABB, as a math::Box<float>) // Note: I dislike having the node bounding box after the children BB, but it is computed form children's...
    //  numMaterials
    //  raw memory dump of span<PhongMaterial>
    //  numMaterialNames (Note: redundant with num of materials...)
    //  each materialName: (count == numMaterials)
    //    string size
    //    string characters
    //  diffuse numTexturePaths
    //  diffuse texturesUnifiedDimensions (as math::Size<2, int>) //Note: even if there are no textures
    //  each diffuse texturePath:
    //    string size
    //    string characters
    //  normalmap numTexturePaths
    //  normalmap texturesUnifiedDimensions (as math::Size<2, int>) //Note: even if there are no textures
    //  each normalmap texturePath:
    //    string size
    //    string characters

    /// @return AABB of the node, as the union of nested nodes and meshes AABBs.
    NodeResult recurseNodes(aiNode * aNode,
                            const aiScene * aScene,
                            FileWriter & aWriter,
                            unsigned int level = 0)
    {
        std::cout << std::string(2 * level, ' ') << "'" << aNode->mName.C_Str() << "'"
                  << ", " << aNode->mNumMeshes << " mesh(es)"
                  << ", " << aNode->mNumChildren << " child(ren)"
                  << "\n"
                  ;

        aWriter.write(aNode);

        NodeResult result;
        // Prime this node's bounding box
        if(aNode->mNumMeshes != 0)
        {
            result.mAabb = extractAabb(aScene->mMeshes[0]);
        }

        for(std::size_t meshIdx = 0; meshIdx != aNode->mNumMeshes; ++meshIdx)
        {
            unsigned int globalMeshIndex = aNode->mMeshes[meshIdx];
            aiMesh * mesh = aScene->mMeshes[globalMeshIndex];
            assert(mesh->HasPositions() && mesh->HasFaces());
            assert(hasTrianglesOnly(mesh));

            math::Box<float> meshAabb = extractAabb(mesh);
            result.mAabb.uniteAssign(meshAabb);
            result.mVerticesCount += mesh->mNumVertices;
            result.mIndicesCount += mesh->mNumFaces * 3;

            std::cout << std::string(2 * level, ' ')
                << "- Mesh " << globalMeshIndex << " '" << mesh->mName.C_Str() << "'" << " with " << mesh->mNumVertices << " vertices, " 
                << mesh->mNumFaces << " triangles."
                << "\n"
                << std::string(2 * level + 2, ' ') << "- " << mesh->GetNumColorChannels() << " color channel(s), "
                    << mesh->GetNumUVChannels() << " UV channel(s)."
                << "\n"
                << std::string(2 * level + 2, ' ') << "- AABB " << meshAabb << "."
                << "\n"
                ;

            aWriter.write(mesh);
            aWriter.forward(meshAabb);
        }

        aWriter.forward(result.mAabb); // this is the AABB of all direct parts, without children nodes. (i.e the `Object`).

        // TODO do we really want to compute the node's AABB? This is tricky, because there might be transformations
        // between nodes.
        // Yet the client will usually be interested in the top node AABB to define camera parameters...
        for(std::size_t childIdx = 0; childIdx != aNode->mNumChildren; ++childIdx)
        {
            NodeResult childResult = 
                recurseNodes(aNode->mChildren[childIdx], aScene, aWriter, level + 1);
            if(childIdx == 0 && aNode->mNumMeshes == 0) // The box was not primed yet
            {
                result.mAabb = childResult.mAabb;
            }
            else
            {
                result.mAabb.uniteAssign(childResult.mAabb);
            }

            result.mVerticesCount += childResult.mVerticesCount;
            result.mIndicesCount  += childResult.mIndicesCount;
        }

        // This is now the AABB including the children nodes.
        aWriter.forward(result.mAabb);

        return result;
    }


    void set(const aiColor4D & aSource, math::hdr::Rgba_f & aDestination)
    {
        aDestination.r() = aSource.r;
        aDestination.g() = aSource.g;
        aDestination.b() = aSource.b;
        aDestination.a() = aSource.a;
    }

    template <class... VT_args>
    void setColor(math::hdr::Rgba_f & aDestination, aiMaterial * aMaterial, VT_args &&... aArgs)
    {
        aiColor4D aiColor;
        if(aMaterial->Get(aArgs..., aiColor) == AI_SUCCESS)
        {
            set(aiColor, aDestination);
            std::cout << "  color '" << std::get<0>(std::forward_as_tuple(aArgs...)) << "': " << aDestination << "\n";
        }
    }


    math::Size<2, int> resampleImages(std::vector<std::string> & aTexturePaths, filesystem::path aParentPath)
    {
        using Image = arte::Image<math::sdr::Rgba>;

        // Find max dimension
        int maxDimension = 0;
        for(auto imagePath = aTexturePaths.begin(); imagePath != aTexturePaths.end(); ++imagePath)
        {
            auto dimensions = Image{aParentPath / *imagePath}.dimensions();
            maxDimension = std::max(maxDimension, *dimensions.getMaxMagnitudeElement());

            if(dimensions.width() != dimensions.height())
            {
                SELOG(warn)("Texture image {} does not have square dimensions {}.",
                            *imagePath, fmt::streamed(dimensions));
            }
        }

        // Resample what is needed
        math::Size<2, int> commonDimensions = {maxDimension, maxDimension};
        for(auto imagePath = aTexturePaths.begin(); imagePath != aTexturePaths.end(); ++imagePath)
        {
            filesystem::path path{*imagePath};
            Image image{aParentPath / path};

            if(image.dimensions().width() < maxDimension || image.dimensions().height() < maxDimension)
            {
                SELOG(info)("Resampling image {} to {}.", *imagePath, fmt::streamed(commonDimensions));

                path = path.parent_path() / "upsampled" / path.filename();
                resampleImage(image, commonDimensions).saveFile(aParentPath / path);
                *imagePath = path.string();
            }
        }
        return commonDimensions;
    }


    void dumpTextures(std::vector<std::string> & aTexturePaths, FileWriter & aWriter)
    {
        using Image = arte::Image<math::sdr::Rgba>;

        filesystem::path basePath = aWriter.mArchive.mParentPath;

        math::Size<2, int> commonDimensions;
        if(!aTexturePaths.empty())
        {
            filesystem::path baseDirAbsolute =  (basePath / aTexturePaths.front()).parent_path();
            filesystem::path upsampledDir = "upsampled";
            filesystem::path upsampledDirAbsolute = baseDirAbsolute / upsampledDir;
            if(is_directory(upsampledDirAbsolute))
            {
                bool foundUpsampledImage = false;
                // Fixup paths for resampled images
                for(auto & texture : aTexturePaths)
                {
                    auto textureFilename = filesystem::path{texture}.filename();
                    auto upsampledTextureAbsolute = upsampledDirAbsolute / textureFilename;
                    // Why is it prepending, relative is actually not relative...
                    // Actually, we should make it relative
                    if(is_regular_file(upsampledTextureAbsolute))
                    {
                        if(!foundUpsampledImage)
                        {
                            commonDimensions = Image{upsampledTextureAbsolute}.dimensions();
                            SELOG(info)("Upsampled images already found, with dimensions {}.",
                                        fmt::streamed(commonDimensions));
                            foundUpsampledImage = true;
                        }
                        // Replace the base texture relative path with the upsampled relative path
                        texture = (upsampledDir / textureFilename).string();
                    }
                }

                if(!foundUpsampledImage)
                {
                    SELOG(critical)("'{}' directory did not contain any expected texture.", upsampledDir.string());
                    throw std::runtime_error("Upsampling directory cannot be left empty, please remove it.");
                }
            }
            else
            {
                create_directory(upsampledDirAbsolute);
                commonDimensions = resampleImages(aTexturePaths, basePath);
                filesystem::remove(upsampledDirAbsolute); // this function only removes empty directories
            }
        }

        aWriter.forward(commonDimensions);
        aWriter.write(aTexturePaths);
    }


    TextureInput readTextureParameters(const aiMaterial * aAiMaterial, 
                                       aiTextureType aTextureType,
                                       std::vector<std::string> & aTexturePaths)
    {
        // TODO: handle several textures in the stacks?
        assert(aAiMaterial->GetTextureCount(aTextureType) <= 1);

        TextureInput result;

        // Consistent with max 1 texture in the stack
        constexpr unsigned int indexInStack = 0;
        aiString texPath;
        if(aAiMaterial->Get(_AI_MATKEY_TEXTURE_BASE, aTextureType, indexInStack, texPath) == AI_SUCCESS)
        {
            std::cout << "  " << aiTextureTypeToString(aTextureType) << " texture: path '" << texPath.C_Str() << "'";

            result = {
                .mTextureIndex = (TextureInput::Index)aTexturePaths.size(),
                .mUVAttributeIndex = 0, // a default,
                                        // see: https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#how-to-map-uv-channels-to-textures-matkey-uvwsrc
            };
            aTexturePaths.push_back(texPath.C_Str());

            unsigned int aiIndex;
            if(aAiMaterial->Get(_AI_MATKEY_UVWSRC_BASE, aTextureType, indexInStack, aiIndex) == AI_SUCCESS)
            {
                result.mUVAttributeIndex = aiIndex;
                std::cout << ", explicit UV channel " << aiIndex;
            }
            else
            {
                std::cout << ", implicit UV channel " << result.mUVAttributeIndex;
            }

            std::cout << "\n"; // Terminate the output which started entering this scope
        }
        return result;
    }


    void dumpMaterials(const aiScene * aScene, FileWriter & aWriter)
    {
        std::vector<PhongMaterial> materials;
        materials.reserve(aScene->mNumMaterials);

        std::vector<std::string> diffuseTexturePaths;
        std::vector<std::string> normalTexturePaths;
        std::vector<std::string> materialNames;

        for(std::size_t materialIdx = 0; materialIdx != aScene->mNumMaterials; ++materialIdx)
        {
            materials.emplace_back();
            PhongMaterial & phongMaterial = materials.back();

            aiMaterial * material = aScene->mMaterials[materialIdx];

            materialNames.emplace_back(material->GetName().C_Str());

            std::cout << "Material '" << material->GetName().C_Str()
                << "' Diffuse tex:" << material->GetTextureCount(aiTextureType_DIFFUSE)
                << " Specular tex:" << material->GetTextureCount(aiTextureType_SPECULAR)
                << " Ambient tex:" << material->GetTextureCount(aiTextureType_AMBIENT)
                << " Normal map:" << material->GetTextureCount(aiTextureType_NORMALS)
                << std::endl;

            // TODO Handle specular and ambient textures
            if(material->GetTextureCount(aiTextureType_SPECULAR) != 0)
            {
                SELOG(warn)("Material '{}' has {} specular textures, but exporter does not handle them yet.",
                    material->GetName().C_Str(), material->GetTextureCount(aiTextureType_SPECULAR));
            }
            if(material->GetTextureCount(aiTextureType_AMBIENT) != 0)
            {
                SELOG(warn)("Material '{}' has {} ambient textures, but exporter does not handle them yet.",
                    material->GetName().C_Str(), material->GetTextureCount(aiTextureType_AMBIENT));
            }

            setColor(phongMaterial.mDiffuseColor,  material, AI_MATKEY_COLOR_DIFFUSE);
            // Default other colors to the diffuse colors (in case they are not directly defined)
            phongMaterial.mAmbientColor = phongMaterial.mDiffuseColor;
            phongMaterial.mSpecularColor = phongMaterial.mDiffuseColor;
            setColor(phongMaterial.mAmbientColor,  material, AI_MATKEY_COLOR_AMBIENT);
            setColor(phongMaterial.mSpecularColor, material, AI_MATKEY_COLOR_SPECULAR);

            phongMaterial.mDiffuseMap = readTextureParameters(material, aiTextureType_DIFFUSE, diffuseTexturePaths);
            phongMaterial.mNormalMap  = readTextureParameters(material, aiTextureType_NORMALS, normalTexturePaths);

            if(material->Get(AI_MATKEY_SHININESS, phongMaterial.mSpecularExponent) == AI_SUCCESS)
            {
                // Correct the specular exponent if needed
                if(phongMaterial.mSpecularExponent < 1)
                {
                    SELOG(warn)("Specular exponent value '{}' is too low, setting it to '1'.",
                                phongMaterial.mSpecularExponent);
                    phongMaterial.mSpecularExponent = 1;
                }
                std::cout << "  specular exponent: " << phongMaterial.mSpecularExponent << "\n";
            }

            if(material->Get(AI_MATKEY_OPACITY, phongMaterial.mDiffuseColor.a()) == AI_SUCCESS)
            {
                std::cout << "  opacity factor: " << phongMaterial.mDiffuseColor.a() << "\n";
            }
            else if(material->Get(AI_MATKEY_TRANSPARENCYFACTOR, phongMaterial.mDiffuseColor.a()) == AI_SUCCESS)
            {
                std::cout << "  transparency factor: " << phongMaterial.mDiffuseColor.a() << "\n";
                phongMaterial.mDiffuseColor.a() = 1 - phongMaterial.mDiffuseColor.a();
            }

            normalizeColorFactors(phongMaterial);
        }

        aWriter.writeRaw(std::span{materials});
        aWriter.write(materialNames);
        dumpTextures(diffuseTexturePaths, aWriter);
        dumpTextures(normalTexturePaths, aWriter);
    }

} // unnamed namespace


void processModel(const std::filesystem::path & aFile)
{
    // Comment out to get verbose output from the importer, to stdout.
    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);

    // Create an instance of the Importer class
    Assimp::Importer importer;

    // This is really extra verbose, usually the default logger verbository
    //importer.SetExtraVerbose(true); 

    // Have the importer read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll 
    // propably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile(
        aFile.string(),
        aiProcess_CalcTangentSpace       | 
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType            |
        // Ad: added flags below
        aiProcess_RemoveRedundantMaterials  |
        aiProcess_GenSmoothNormals          |
        aiProcess_ValidateDataStructure     |
        aiProcess_FindDegenerates           |
        // Note: Will remove "invalid" channels (e.g. all zero normals / tangents)
        // This will causes issues when the UV / Tangent are not valid: it removes them
        // which trips the `Processor` atm.
        aiProcess_FindInvalidData           |
        // Generate bouding boxes, see: https://stackoverflow.com/a/74331859/1027706
        aiProcess_GenBoundingBoxes          |
        /* to allow final | */0
    );
    
    // If the import failed, report it
    if(!scene)
    {
        SELOG(critical)(importer.GetErrorString());
        return;
    }

    std::filesystem::path output = aFile.parent_path() / aFile.stem().replace_extension(".seum");
    FileWriter writer{output};

    // Reserve room for the total vertex count
    auto preamblePosition = reservePreamble(writer);

    // Now we can access the file's contents. 
    NodeResult topResult = recurseNodes(scene->mRootNode, scene, writer);

    completePreamble(writer, preamblePosition, topResult);

    dumpMaterials(scene, writer);

    std::cout << "\nResult: Dumped model with "
              << topResult.mVerticesCount << " vertices and " 
              << topResult.mIndicesCount << " indices, "
              << scene->mNumMaterials << " materials."
              << "\n  " << "bounding box: " << topResult.mAabb
              << "\n";

    // We're done. Everything will be cleaned up by the importer destructor
}


} // namespace ad::renderer