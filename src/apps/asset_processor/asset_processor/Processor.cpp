#include "Processor.h"

#include "Logging.h"

// TODO address that dirty include when the content is moved to a proper library
#include "../../refactor_proto/refactor_proto/BinaryArchive.h"
#include "../../refactor_proto/refactor_proto/Material.h"

#include <math/Homogeneous.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <fstream>
#include <iostream>
#include <span>


namespace ad::renderer {

namespace {

    bool hasTrianglesOnly(aiMesh * aMesh)
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

    math::AffineMatrix<4, float> extractTransformation(const aiNode * aNode)
    {
        return math::AffineMatrix<4, float>{extractAffinePart(aNode)};
    }
    
    struct FileWriter
    {
        FileWriter(std::filesystem::path aDestinationFile) :
            mArchive{
                .mOut = std::ofstream{aDestinationFile, std::ios::binary}
            }
        {
            unsigned int version = 1;
            mArchive.write(version);
        }

        void write(const aiNode * aNode)
        {
            mArchive.write(aNode->mNumMeshes);
            mArchive.write(aNode->mNumChildren);
            mArchive.write(extractAffinePart(aNode));
        }

        void write(const aiMesh * aMesh)
        {
            //TODO Ad 2023/07/27: It could be better to dump only the materials actually references by meshes
            // (requires a step to re-index without the unused materials)
            // Material index
            mArchive.write(aMesh->mMaterialIndex);

            // Vertices
            mArchive.write(aMesh->mNumVertices);
            mArchive.write(std::span{aMesh->mVertices, aMesh->mNumVertices});

            assert(aMesh->mNormals != nullptr);
            mArchive.write(std::span{aMesh->mNormals, aMesh->mNumVertices});

            assert(aMesh->GetNumColorChannels() == 0); // TODO handle this if some assets use vertex colors

            mArchive.write(aMesh->GetNumUVChannels());
            for (unsigned int uvIdx = 0; uvIdx != aMesh->GetNumUVChannels(); ++ uvIdx)
            {
                // Only support bidimensionnal texture sampling atm.
                // Sadly, some models have a 3 here, even if they actually use only 2 (e.g. teapot.obj)
                //assert(aMesh->mNumUVComponents[uvIdx] == 2);
                // TODO Interleave the odd channel with their preceding even channel
                // (i.e. better usage of the 4 components of each vertex attribute)
                for(auto vertexIdx = 0; vertexIdx != aMesh->mNumVertices; ++vertexIdx)
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

        BinaryOutArchive mArchive;
    };

    // Current binary format as of 2023/07/25:
    //  node.numMeshes
    //  node.numChildren
    //  node.transformation
    //  each node.mesh:
    //    mesh.materialIndex
    //    mesh.numVertices
    //    [mesh.vertices(i.e. positions, 3 floats per vertex)]
    //    [mesh.normals(3 floats per vertex)]
    //    mesh.numUVChannels
    //    each mesh.UVChannel:
    //      [UVChannel.coordinates(2 floats per vertex)]
    //    mesh.numFaces
    //    [mesh.faces(i.e. 3 unsigned int per face)]
    //  each node.child:
    //    // recurse
    //  numMaterials
    //  raw memory dump of span<PhongMaterial>
    //  numTexturePaths
    //  each texturePath:
    //    string size
    //    string characters
    void recurseNodes(aiNode * aNode,
                      const aiScene * aScene,
                      FileWriter & aWriter,
                      unsigned int level = 0)
    {
        math::AffineMatrix<4, float> nodeTransform = extractTransformation(aNode);

        //SELOG(info)("{}{}, {} mesh(es).",
        //            std::string(2 * level, ' '), aNode->mName.C_Str(), aNode->mNumMeshes);
        std::cout << std::string(2 * level, ' ') << aNode->mName.C_Str() 
                  << ", " << aNode->mNumMeshes << " mesh(es)"
                  << "\n" << nodeTransform 
                  << "\n"
                  ;

        aWriter.write(aNode);

        for(std::size_t meshIdx = 0; meshIdx != aNode->mNumMeshes; ++meshIdx)
        {
            unsigned int globalMeshIndex = aNode->mMeshes[meshIdx];
            aiMesh * mesh = aScene->mMeshes[globalMeshIndex];
            assert(mesh->HasPositions() && mesh->HasFaces());
            assert(hasTrianglesOnly(mesh));

            std::cout << std::string(2 * level, ' ')
                << "- Mesh " << globalMeshIndex << " with " << mesh->mNumVertices << " vertices, " 
                << mesh->mNumFaces << " triangles."
                << "\n"
                ;

            aWriter.write(mesh);
        }

        for(std::size_t childIdx = 0; childIdx != aNode->mNumChildren; ++childIdx)
        {
            recurseNodes(aNode->mChildren[childIdx], aScene, aWriter, level + 1);
        }
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
        }
    }


    void dumpMaterials(const aiScene * aScene, FileWriter & aWriter)
    {
        std::vector<PhongMaterial> materials;
        materials.reserve(aScene->mNumMaterials);

        std::vector<std::string> texturePaths;

        for(std::size_t materialIdx = 0; materialIdx != aScene->mNumMaterials; ++materialIdx)
        {
            materials.emplace_back();
            PhongMaterial & phongMaterial = materials.back();

            aiMaterial * material = aScene->mMaterials[materialIdx];

            std::cout << "Material '" << material->GetName().C_Str()
                << "' Diffuse tex:" << material->GetTextureCount(aiTextureType_DIFFUSE)
                << " Specular tex:" << material->GetTextureCount(aiTextureType_SPECULAR)
                << " Ambient tex:" << material->GetTextureCount(aiTextureType_AMBIENT)
                << std::endl;

            // TODO Handle other textures than diffuse.
            // (and handle several textures in the stacks?)
            assert(material->GetTextureCount(aiTextureType_SPECULAR) == 0
                && material->GetTextureCount(aiTextureType_AMBIENT) == 0
                && material->GetTextureCount(aiTextureType_DIFFUSE) <= 1);

            setColor(phongMaterial.mAmbientColor,  material, AI_MATKEY_COLOR_AMBIENT);
            setColor(phongMaterial.mDiffuseColor,  material, AI_MATKEY_COLOR_DIFFUSE);
            setColor(phongMaterial.mSpecularColor, material, AI_MATKEY_COLOR_SPECULAR);

            constexpr unsigned int indexInStack = 0;
            aiString texPath;
            if(material->Get(AI_MATKEY_TEXTURE_DIFFUSE(indexInStack), texPath) == AI_SUCCESS)
            {
                std::cout << "  diffuse texture path: '" << texPath.C_Str() << "'\n";
                phongMaterial.mDiffuseMap = {
                    .mTextureIndex = (TextureInput::Index)texturePaths.size(),
                    .mUVAttributeIndex = 0, // a default,
                                            // see: https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#how-to-map-uv-channels-to-textures-matkey-uvwsrc
                };
                texturePaths.push_back(texPath.C_Str());
            }

            unsigned int aiIndex;
            if(material->Get(AI_MATKEY_UVWSRC_DIFFUSE(indexInStack), aiIndex) == AI_SUCCESS)
            {
                phongMaterial.mDiffuseMap.mUVAttributeIndex = aiIndex;
            }
        }

        aWriter.writeRaw(std::span{materials});
        aWriter.write(texturePaths);
    }

} // unnamed namespace


void processModel(const std::filesystem::path & aFile)
{
    // Create an instance of the Importer class
    Assimp::Importer importer;
    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll 
    // propably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile(
        aFile.string(),
          aiProcess_CalcTangentSpace       | 
          aiProcess_Triangulate            |
          aiProcess_JoinIdenticalVertices  |
          aiProcess_SortByPType            |
          // Ad: added flags below
          aiProcess_RemoveRedundantMaterials |
          aiProcess_GenSmoothNormals         |
          aiProcess_ValidateDataStructure    |
          aiProcess_FindDegenerates          |
          aiProcess_FindInvalidData
    );
    
    // If the import failed, report it
    if(!scene)
    {
        SELOG(critical)(importer.GetErrorString());
        return;
    }

    std::filesystem::path output = aFile.parent_path() / aFile.stem().replace_extension(".seum");
    FileWriter writer{output};

    // Now we can access the file's contents. 
    recurseNodes(scene->mRootNode, scene, writer);

    dumpMaterials(scene, writer);

    // We're done. Everything will be cleaned up by the importer destructor
}


} // namespace ad::renderer