#include "Processor.h"

#include "Logging.h"

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

    math::AffineMatrix<4, float> extractTransformation(aiNode * aNode)
    {
        auto m = aNode->mTransformation;
        math::Matrix<4, 3, float> affinePart{
            m.a1, m.b1, m.c1,
            m.a2, m.b2, m.c2,
            m.a3, m.b3, m.c3,
            m.a4, m.b4, m.c4,
        };
        return math::AffineMatrix<4, float>{affinePart};
    }

    
    struct BinaryArchive
    {
        template <class T> requires std::is_arithmetic_v<T>
        BinaryArchive & write(T aValue)
        {
            mOut.write((const char *)&aValue, sizeof(T));
            return *this;
        }

        template <class T>
        BinaryArchive & write(std::span<T> aData)
        {
            mOut.write((const char *)aData.data(), aData.size_bytes());
            return *this;
        }

        std::ofstream mOut;
    };

    struct FileWriter
    {
        void write(const aiMesh * aMesh)
        {
            // Vertices
            mArchive.write(aMesh->mNumVertices);
            mArchive.write(std::span{aMesh->mVertices, aMesh->mNumVertices});
            // Faces
            mArchive.write(aMesh->mNumFaces);
            for(std::size_t faceIdx = 0; faceIdx != aMesh->mNumFaces; ++faceIdx)
            {
                const aiFace & face = aMesh->mFaces[faceIdx];
                mArchive.write(std::span{face.mIndices, 3});
            }
        }

        BinaryArchive mArchive;
    };

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
          aiProcess_ValidateDataStructure
          );
    
    // If the import failed, report it
    if(!scene)
    {
        SELOG(critical)(importer.GetErrorString());
        return;
    }

    std::filesystem::path output = aFile.parent_path() / aFile.stem().replace_extension(".seum");
    FileWriter writer{
        .mArchive{
            .mOut = std::ofstream{output, std::ios::binary},
        }
    };

    // Now we can access the file's contents. 
    recurseNodes(scene->mRootNode, scene, writer);
    // We're done. Everything will be cleaned up by the importer destructor
}


} // namespace ad::renderer