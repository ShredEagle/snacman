#include "Processor.h"

#include "Logging.h"

// TODO address that dirty include when the content is moved to a proper library
#include "../../refactor_proto/refactor_proto/BinaryArchive.h"

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
            // Vertices
            mArchive.write(aMesh->mNumVertices);
            mArchive.write(std::span{aMesh->mVertices, aMesh->mNumVertices});
            assert(aMesh->mNormals != nullptr);
            mArchive.write(std::span{aMesh->mNormals, aMesh->mNumVertices});
            // Faces
            mArchive.write(aMesh->mNumFaces);
            for(std::size_t faceIdx = 0; faceIdx != aMesh->mNumFaces; ++faceIdx)
            {
                const aiFace & face = aMesh->mFaces[faceIdx];
                mArchive.write(std::span{face.mIndices, 3});
            }
        }

        BinaryOutArchive mArchive;
    };

    // Current binary format as of 2023/07/25:
    //  node.numMeshes
    //  node.numChildren
    //  node.transformation
    //  each node.mesh:
    //    mesh.numVertices
    //    [mesh.vertices(i.e. positions, 3 floats per vertex)]
    //    [mesh.normals(3 floats per vertex)]
    //    mesh.numFaces
    //    [mesh.faces(i.e. 3 unsigned int per face)]
    //  each node.child:
    //    // recurse
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
          aiProcess_ValidateDataStructure  |
          aiProcess_GenNormals
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
    // We're done. Everything will be cleaned up by the importer destructor
}


} // namespace ad::renderer