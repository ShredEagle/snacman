#include "Processor.h"

#include "AssimpUtils.h"
#include "Logging.h"
#include "ProcessAnimation.h"

#include <assimp/DefaultLogger.hpp>
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
#include <snac-renderer-V2/files/Versioning.h>

#include <fmt/ostream.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <span>


namespace ad::renderer {

namespace {


    struct FileWriter
    {
        FileWriter(std::filesystem::path aDestinationFile) :
            mArchive{
                .mOut = std::ofstream{aDestinationFile, std::ios::binary},
                .mParentPath = aDestinationFile.parent_path(),
            }
        {
            mArchive.write(gSeumMagic);
            mArchive.write(gSeumVersion);
        }

        void write(const aiNode * aNode)
        {
            mArchive.write(std::string{aNode->mName.C_Str()});
            mArchive.write(aNode->mNumMeshes);
            mArchive.write(aNode->mNumChildren);
            // TODO Should we decompose the transformation to store the decomposed form?
            // (since currently we decompose on load, it would be an optimization)
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
            // Note that with assimp, tangents and bitangents are either both present or both absent.
            aiVector3D * bitangents = aMesh->mBitangents;
            // positions and normals are always required
            unsigned int vertexAttributesFlags = gVertexPosition | gVertexNormal;
            // Note: For the moment, since the structure of the loader is too rigid
            // the tangents are mandatory (the vertexAttributesFlags is an illusion)
            // TODO Ad 2024/02/21: #assetprocessor Can we handle "dynamic" list of vertex attributes?
            // (e.g., making tangents optional)
            assert(tangents);
            if(tangents) vertexAttributesFlags |= gVertexTangent;

            assert(bitangents); // Note: we should never fail here: if tangents were presents,
                                // assimp guarantees bitangents are present
            if(bitangents) vertexAttributesFlags |= gVertexBitangent;

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
            if(bitangents)
            {
                mArchive.write(std::span{bitangents, aMesh->mNumVertices});
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


        void write(const RigAnimation & aAnimation)
        {
            forward(aAnimation.mName);
            forward(aAnimation.mDuration);
            writeRaw(std::span{aAnimation.mTimepoints});
            writeRaw(std::span{aAnimation.mNodes});

            for(const RigAnimation::NodeKeyframes & keyframes : aAnimation.mKeyframes)
            {
                forward(std::span{keyframes.mTranslations});
                forward(std::span{keyframes.mRotations});
                forward(std::span{keyframes.mScales});
            }
        }


        void write(const std::string & aString)
        {
            forward(aString);
        }

        template<class T_element>
        void write(const std::vector<T_element> & aVector)
        {
            mArchive.write((unsigned int)aVector.size());
            for(const auto & element : aVector)
            {
                write(element);
            }
        }

        // TODO Ad 2024/02/29: rename
        template <class T_element>
        void writeRaw(std::span<T_element> aData)
        {
            mArchive.write((unsigned int)aData.size());
            mArchive.write(aData);
        }

        // TODO this feels out of place, this file writer being intended to be high level
        template <class T_data>
        void forward(const T_data & aData)
        {
            mArchive.write(aData);
        }

        BinaryOutArchive mArchive;
    };
    

    // Internal class, maintaining the state of the Rig created by parsing a given node.
    struct NodeRig
    {
        aiNode * mCommonArmature;
        Rig mRig;
        JointDataDeduplicate mJointDataDeduplicator;
        NodePointerMap mAiNodeToTreeNode;
        std::unordered_map<std::string, NodeTree<Rig::Pose>::Node::Index> mNameToTreeNode;
    };

    /// @brief Return type on "visiting" a node (i.e. recurseNode())
    struct NodeResult
    {
        math::Box<float> mAabb;
        unsigned int mVerticesCount = 0;
        unsigned int mIndicesCount = 0;

        // #singlerig At most a single Rig accross the whole scene is supported ATM.
        std::optional<NodeRig> mNodeRig;
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
    //      <if mesh.hasBitangents>: [mesh.bitangents(3 floats per vertex)]
    //      mesh.numColorChannels
    //      each mesh.ColorChannel:
    //        [ColorChannel.color(4 floats per vertex)]
    //      mesh.numUVChannels
    //      each mesh.UVChannel:
    //        [UVChannel.coordinates(2 floats per vertex)]
    //      mesh.numFaces
    //      [mesh.faces(i.e. 3 unsigned int per face)]
    //      mesh.numBones
    //      <if mesh.hasBones>:
    //          [mesh.jointData (i.e 4 bone indices (unsigned int) then 4 bone weights (float), per vertex)]
    //      mesh.boundingBox (AABB, as a math::Box<float>)
    //    node.rigCount <currently, only 0 or 1>// computed from the meshes with bones
    //    each node.Rig <at most 1>:
    //      rig.mJointTree:
    //        jointTree.numNodes
    //        raw memory dump of span<Node>
    //        jointTree.firstRootIndex
    //        raw memory dump of span<LocalPose>
    //        raw memory dump of span<GlobalPose>
    //        jointTree.nodeNames
    //      rig.mJoints:
    //        numJoints
    //        raw memory dump of span<Node::Index> // 1 per joint
    //        raw memory dump of span<InverseBindMatrices> // 1 per joint
    //    bounding box union of all meshes (AABB, as a math::Box<float>, even if there are no meshes)
    //    each node.child:
    //      // recurse Node:
    //    node.boundingBox (AABB, as a math::Box<float>) // Note: I dislike having the node bounding box after the children BB, but it is computed form children's...
    //  numAnimations
    //  each RigAnimation:
    //    name string size
    //    name string characters
    //    duration
    //    num keyframes (== num of timepoints)
    //    raw dump of timepoints
    //    num nodes
    //    raw dump of node indices
    //    each node:
    //      [keyframes.positions (3 floats per keyframe)]
    //      [keyframes.rotations (4 floats per keyframe)]
    //      [keyframes.scalings  (3 floats per keyframe)]
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

        // NOTE Ad 2024/02/28: This is not a hard requirement (code should work without it)
        //   but this situation would exacerbate a design flaw (see note #flaw_593)
        assert(aNode->mNumMeshes == 0 || aNode->mNumChildren == 0);

        std::optional<NodeRig> nodeRig;

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
                << "- Mesh " << globalMeshIndex << " '" << mesh->mName.C_Str() << "'" 
                    << " with " << mesh->mNumVertices << " vertices, " << mesh->mNumFaces << " triangles"
                    << ", material '" << aScene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str() 
                    << "'."
                << "\n"
                << std::string(2 * level + 2, ' ') << "- Normals: " << (mesh->HasNormals() ? "yes" : "no")
                    << ", tangents & bitangents: " << (mesh->HasTangentsAndBitangents() ? "yes" : "no")
                << "\n"
                << std::string(2 * level + 2, ' ') << "- " << mesh->GetNumColorChannels() << " color channel(s), "
                    << mesh->GetNumUVChannels() << " UV channel(s)."
                << "\n" << std::string(2 * level + 2, ' ') << "- "
                    << mesh->mNumBones << " bones."
                << "\n"
                << std::string(2 * level + 2, ' ') << "- AABB " << meshAabb << "."
                << "\n"
                ;

            aWriter.write(mesh);

            //
            // Skeletal animation data
            //
            aWriter.forward(mesh->mNumBones);
            if(mesh->mNumBones != 0)
            {
                aiNode * meshArmature = mesh->mBones[0]->mArmature;
                assert(meshArmature);
                if(!nodeRig) 
                {
                    nodeRig = NodeRig{
                        .mCommonArmature = meshArmature,
                    };
                    std::tie(nodeRig->mRig, nodeRig->mAiNodeToTreeNode) = loadRig(meshArmature);
                    traverseDepth(
                        nodeRig->mRig.mJointTree, 
                        [& nameMap = nodeRig->mNameToTreeNode]
                        (NodeTree<Rig::Pose>::Node::Index aNodeIdx, NodeTree<Rig::Pose> & aTree)
                        {
                            assert(aTree.hasName(aNodeIdx));
                            nameMap.emplace(aTree.mNodeNames.at(aNodeIdx), aNodeIdx);
                        });
                }
                // The code is written assuming all (rigged) meshes of the same node have the same armature.
                // If this assertion fails, code needs reviewing.
                // (in particular the fact that a single rig can be assigned to a Node)
                assert(meshArmature == nodeRig->mCommonArmature);

                const std::vector<VertexJointData> jointData = 
                    populateJointData(nodeRig->mJointDataDeduplicator,
                                      nodeRig->mRig.mJoints,
                                      mesh,
                                      nodeRig->mAiNodeToTreeNode,
                                      nodeRig->mCommonArmature);

                aWriter.forward(std::span{jointData});
            }

            aWriter.forward(meshAabb);
        }

        // Rig of the Node
        {
            // Node rig count (can only have one at the moment)
            unsigned int rigCount = nodeRig ? 1 : 0;
            aWriter.forward(rigCount);
            if(nodeRig)
            {
                result.mNodeRig = nodeRig;

                const Rig & rig = nodeRig->mRig;
// Run some more sanity checks on the Rig
#if not defined(NDEBUG)
                // Let's ensure that all bones, from the different meshes,
                // point to distinct Nodes in the hierarchy.
                const auto & indices = rig.mJoints.mIndices;
                std::vector<NodeTree<Rig::Pose>::Node::Index> sortedIndices(indices.size(), 0);
                std::partial_sort_copy(indices.begin(), indices.end(),
                                       sortedIndices.begin(), sortedIndices.end());
                
                // If this trips, it means several bones pointed to the same aiNode,
                // and this should never be the case because we take explicit measures to implement bone deduplication.
                // Note: Joint duplicates happen in the Assimp model because of this design:
                // The Bones are stored in the meshes.
                // So, if several meshes are influenced by the same bone in a logical skeleton,
                // each meach probably stores a bone to reference to the same node.
                assert(std::adjacent_find(sortedIndices.begin(), sortedIndices.end()) == sortedIndices.end()
                    && "Duplicate bones found in the rig");
#endif
                // Joint Tree
                const NodeTree<Rig::Pose> & jointTree = rig.mJointTree;
                // Writes the number of elements first
                aWriter.writeRaw(std::span{jointTree.mHierarchy});
                aWriter.forward(jointTree.mFirstRoot);
                // Does not write number of elements (will be the same)
                aWriter.forward(std::span{jointTree.mLocalPose});
                aWriter.forward(std::span{jointTree.mGlobalPose});
                aWriter.forward(jointTree.mNodeNames);

                // Joint Data
                // Writes the number of joints first
                aWriter.writeRaw(std::span{rig.mJoints.mIndices});
                aWriter.forward(std::span{rig.mJoints.mInverseBindMatrices});

                // Armature name
                aWriter.forward(rig.mArmatureName);
            }
        }

        // AABB of the Node
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
            if(childResult.mNodeRig)
            {
                // NOTE Ad 2024/03/06: This is currently a **hard** requirement.
                //   #animation #singlerig Assimp data-model keeps "Animation" decoupled from nodes (listed under the global scene),
                //   and it does not really have a notion of Rig.
                //   Currently, we assume all animations are targeting the single rig we allow.
                assert(!result.mNodeRig);
                result.mNodeRig = childResult.mNodeRig;
            }
        }

        // This is now the AABB including the children nodes.
        aWriter.forward(result.mAabb);

        return result;
    }


    void dumpAnimations(const aiScene * aScene, const NodeRig & aExtractedRig, FileWriter & aWriter)
    {
        using Node = NodeTree<Rig::Pose>::Node;

        std::vector<RigAnimation> animations;

        // Allows to assert that scaling is uniform, while doing the conversion
        auto scaleConverter = [](aiVector3D aScaleVec)
        {
            auto result = toVec(aScaleVec);
            // We require uniform scaling at the moment.
            // (no specific reason atm, but let's be conservative for when we want to decompose)
            assert(math::relativeTolerance(result.x(), result.y(), 0.0001f)
                && math::relativeTolerance(result.x(), result.z(), 0.0001f));
            return result;
        };

        for(unsigned int animationIdx = 0; animationIdx != aScene->mNumAnimations; ++animationIdx)
        {
            const aiAnimation * animation = aScene->mAnimations[animationIdx];
            assert(animation->mTicksPerSecond != 0); // Means it would be absent from the imported file

            const float duration = (float)(animation->mDuration / animation->mTicksPerSecond);
            std::cout << "Animation '" << animation->mName.C_Str() << "'"
                << ", " << "duration " << duration << "s"
                << " (at " << animation->mTicksPerSecond << " ticks/s)"
                << "\n"
                ;

            // It seems it is not an animation if there are no channels
            assert(animation->mNumChannels > 0);

            RigAnimation & rigAnimation = animations.emplace_back(RigAnimation{
                .mName = animation->mName.C_Str(),
                .mDuration = duration,
            });

            // Compute the number of keyframes in the animation
            unsigned int animationNumKeyframes = 0;
            for(unsigned int channelIdx = 0; channelIdx != animation->mNumChannels; ++channelIdx)
            {
                const aiNodeAnim * channel = animation->mChannels[channelIdx];
                animationNumKeyframes = 
                    std::max(animationNumKeyframes,
                        std::max(channel->mNumPositionKeys, 
                            std::max(channel->mNumRotationKeys, channel->mNumScalingKeys)));
            }
            assert(animationNumKeyframes != 0);

            unsigned int animatedNodes = 0;
            for(unsigned int channelIdx = 0; channelIdx != animation->mNumChannels; ++channelIdx)
            {
                const aiNodeAnim * channel = animation->mChannels[channelIdx];

                ++animatedNodes;
                Node::Index treeNodeIdx = aExtractedRig.mNameToTreeNode.at(channel->mNodeName.C_Str());
                rigAnimation.mNodes.push_back(treeNodeIdx);

                RigAnimation::NodeKeyframes & keyframes = rigAnimation.mKeyframes.emplace_back();
                keyframes.reserve(animationNumKeyframes);

                auto populateKeyframes = 
                    [animationNumKeyframes, &rigAnimation, ticksPerSecond = (float)animation->mTicksPerSecond]
                    (auto & keyframesArray, unsigned int keyCount, auto keys, const auto & converter)
                    {
                        if(keyCount == 1) // We consider it to mean "not animated"
                        {
                            auto key = keys[0];
                            assert(key.mTime == 0.);

                            std::fill_n(std::back_inserter(keyframesArray), 
                                        animationNumKeyframes,
                                        converter(key.mValue));
                        }
                        else
                        {
                            // We require each key to either be not animated (num == 1),
                            // or have the same number of entries as other animated keys.
                            assert(keyCount == animationNumKeyframes);

                            // We expect the first keyframe to be at time zero,
                            // and the last to be at the final time of the animation (i.e. its duration)
                            assert(keys[0].mTime == 0.);
                            assert((float)keys[animationNumKeyframes - 1].mTime / ticksPerSecond 
                                    == rigAnimation.mDuration);

                            if(rigAnimation.mTimepoints.empty())
                            {
                                for(unsigned int keyIdx = 0; keyIdx != animationNumKeyframes; ++keyIdx)
                                {
                                    rigAnimation.mTimepoints.push_back(
                                        (float)keys[keyIdx].mTime / ticksPerSecond);
                                    keyframesArray.push_back(converter(keys[keyIdx].mValue));
                                }
                            }
                            else
                            {
                                // Ideally we could use std::transform, but we also want to assert that time points are matching
                                for(unsigned int keyIdx = 0; keyIdx != animationNumKeyframes; ++keyIdx)
                                {
                                    assert(rigAnimation.mTimepoints[keyIdx] 
                                            == (float)keys[keyIdx].mTime / ticksPerSecond);
                                    keyframesArray.push_back(converter(keys[keyIdx].mValue));
                                }
                            }
                        }
                    };

                populateKeyframes(keyframes.mTranslations, channel->mNumPositionKeys, channel->mPositionKeys, &::ad::renderer::toVec);
                populateKeyframes(keyframes.mRotations, channel->mNumRotationKeys, channel->mRotationKeys, &::ad::renderer::toQuaternion);
                populateKeyframes(keyframes.mScales, channel->mNumScalingKeys, channel->mScalingKeys, scaleConverter);
            }

            // Handle the special case when an animation only has 1 keyframe in *all* of its channels
            if(animationNumKeyframes == 1)
            {
                assert(rigAnimation.mDuration == 0.);
                assert(rigAnimation.mTimepoints.empty());
                rigAnimation.mTimepoints.push_back(0.);
            }

            std::cout << "  * " << animationNumKeyframes << " keyframes posing " << animatedNodes << " nodes\n";
        }
        aWriter.write(animations);
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


    math::Size<2, int> resampleImages(std::vector<std::string> & aTexturePaths, filesystem::path aParentPath, filesystem::path aUpsampledDir)
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

                path = path.parent_path() / aUpsampledDir / path.filename();
                resampleImage(image, commonDimensions).saveFile(aParentPath / path);
                *imagePath = path.string();
            }
        }
        return commonDimensions;
    }


    // TODO Ad 2024/02/22: Rewrite this overly complex function.
    //      Actually, the caching system might be easier to write directly in "resampleImages"
    // This function resample all texture to a common (square) dimension,
    // upsampling images that require it to an "upsampled" subfolder, and directly patching
    // the path of upsampled images in `aTexturePaths`.
    // The complexity got out of hand with the implementation of a cache mechanism,
    // not resampling images that already were.
    void dumpTextures(std::vector<std::string> & aTexturePaths, FileWriter & aWriter)
    {
        using Image = arte::Image<math::sdr::Rgba>;

        filesystem::path basePath = aWriter.mArchive.mParentPath;

        math::Size<2, int> commonDimensions;
        std::optional<math::Size<2, int>> upsampledDimensions;

        if(!aTexturePaths.empty())
        {
            std::vector<std::string> notAlreadyUpsampled;

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
                            upsampledDimensions = Image{upsampledTextureAbsolute}.dimensions();
                            SELOG(info)("Upsampled images already found, with dimensions {}.",
                                        fmt::streamed(*upsampledDimensions));
                            foundUpsampledImage = true;
                        }
                        else
                        {
                            assert(Image{upsampledTextureAbsolute}.dimensions() == *upsampledDimensions);
                        }
                        // Replace the base texture relative path with the upsampled relative path
                        texture = (upsampledDir / textureFilename).string();
                    }
                    else
                    {
                        notAlreadyUpsampled.push_back(texture);
                    }
                }
            }
            else // If there is no upsampled directory, no texture has been upsampled
            {
                notAlreadyUpsampled = aTexturePaths;
            }

            if(!notAlreadyUpsampled.empty())
            {
                create_directory(upsampledDirAbsolute); // Does nothing if the dir already exists
                commonDimensions = resampleImages(notAlreadyUpsampled, basePath, upsampledDir);
                assert(!upsampledDimensions || commonDimensions == *upsampledDimensions);
                std::error_code ec;
                filesystem::remove(upsampledDirAbsolute, ec); // this function only removes empty directories
            }
            else
            {
                assert(upsampledDimensions);
                commonDimensions = *upsampledDimensions;
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
        // Populate the mNode member of aiBone, so we do not have to manually
        // match up on node names
        aiProcess_PopulateArmatureData      |
        // Limit the maximum number of bones affecting a single vertex
        // (default limit is 4)
        aiProcess_LimitBoneWeights          |
        /* to allow final | */0
    );
    
    // If the import failed, report it
    if(!scene)
    {
        SELOG(critical)(importer.GetErrorString());
        return;
    }

    // Uncomment to get a list of all metadata keys and their type
    //const auto & md = *scene->mMetaData;
    //for(unsigned int i = 0; i != md.mNumProperties; ++i)
    //{
    //    int type = md.mValues[i].mType;
    //    SELOG(info)("Prop name: {} of type {}.", md.mKeys[i].C_Str(), type);

    //    //// To get the values, have to declare v of the correct type
    //    //int v = -1000;
    //    //bool getResult = md.Get(md.mKeys[i], v);
    //    //SELOG(warn)("Prop name [{}]:  {} -> {} of type {}.", getResult, md.mKeys[i].C_Str(), v, type);
    //}

    // Note: we have a problem with the FBX scaling, it seems the model vertices position are expressed in centimeters
    // The debug output of the importer state "Debug, T45804: UpdateImporterScale scale set: 0.01"
    // but "UnitScaleFactor" seems to be 1.
    float unitScaleFactor = 1.f;
    if(!scene->mMetaData->Get("UnitScaleFactor", unitScaleFactor))
    {
        SELOG(info)("Could not read 'UnitScaleFactor' metadata.");
    }
    else if(unitScaleFactor != 1.)
    {
        SELOG(warn)("Unit scale factor is {}.", unitScaleFactor);
    }

    std::filesystem::path output = aFile.parent_path() / aFile.stem().replace_extension(".seum");
    FileWriter writer{output};

    // Reserve room for the total vertex count
    auto preamblePosition = reservePreamble(writer);

    // Now we can access the file's contents. 
    NodeResult topResult = recurseNodes(scene->mRootNode, scene, writer);

    completePreamble(writer, preamblePosition, topResult);

    if(auto nodeRig = topResult.mNodeRig)
    {
        dumpAnimations(scene, *nodeRig, writer);
    }
    else
    {
        // Otherwise, there are zero animations
        writer.forward((unsigned int)0);
    }

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