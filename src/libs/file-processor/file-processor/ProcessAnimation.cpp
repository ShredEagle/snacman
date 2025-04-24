#include "ProcessAnimation.h"

#include "AssimpUtils.h"

#include "Logging.h"

#include <assimp/scene.h>

#include <cassert>


namespace ad::renderer {


using NodeIndex = NodeTree<Rig::Pose>::Node::Index;


namespace {

    NodeIndex recurseAssimpNode(const aiNode * aNode,
                                NodeIndex aParentIndex,
                                NodeTree<Rig::Pose> & aOutTree,
                                NodePointerMap & aOutAiNodeToTreeNode)
    {
        NodeIndex nodeIdx = aOutTree.addNode(aParentIndex,
                                             Rig::Pose{extractAffinePart(aNode)});
        aOutTree.mNodeNames[nodeIdx] = aNode->mName.C_Str();
        aOutAiNodeToTreeNode.emplace(aNode, nodeIdx);

        for(std::size_t childIdx = 0; childIdx != aNode->mNumChildren; ++childIdx)
        {
            recurseAssimpNode(aNode->mChildren[childIdx], nodeIdx, aOutTree, aOutAiNodeToTreeNode);
        }

        return nodeIdx;
    }


    std::pair<NodeTree<Rig::Pose>, NodePointerMap> loadJoints(const aiNode * aArmature)
    {
        NodeTree<Rig::Pose> jointTree;

        // We might need to get the armatureToModel transform if armature is not root
        // (Past me put an assertion so future me knows when this has to be done).
        assert(aArmature->mParent == nullptr);

        NodePointerMap aiNodeToTreeNode;
        // TODO #anim should only load the joints instead of the whole scene under the armature.
        // (potential complication if there are non-joints between joints)
        jointTree.mFirstRoot = 
            recurseAssimpNode(aArmature,
                              NodeTree<Rig::Pose>::Node::gInvalidIndex,
                              jointTree,
                              aiNodeToTreeNode);

        return {jointTree, aiNodeToTreeNode};
    }


    /// @brief Utility class to add joint influences on vertices.
    /// It is notably checking that the max number of allowed bones per vertex is not exceeded.
    struct JointDataManager
    {
        using Counter_t = unsigned short;
        static_assert(VertexJointData::gMaxBones < std::numeric_limits<Counter_t>::max());

        JointDataManager(std::size_t aNumVertices) :
            mVertexJointData{aNumVertices},
            mVertexJointNumber(aNumVertices, Counter_t{0})
        {}

        void addJoint(unsigned int aVertexIdx, unsigned int aBoneIdx, float aWeight)
        {
            Counter_t & nextInfluenceIdx = mVertexJointNumber[aVertexIdx];
            assert(nextInfluenceIdx < VertexJointData::gMaxBones);
            mVertexJointData[aVertexIdx].mBoneIndices[nextInfluenceIdx] = aBoneIdx;
            mVertexJointData[aVertexIdx].mBoneWeights[nextInfluenceIdx] = aWeight;
            ++nextInfluenceIdx;
        }

        std::vector<VertexJointData> mVertexJointData;
        std::vector<Counter_t> mVertexJointNumber;
    };


} // unnamed namespace


std::pair<Rig, NodePointerMap> loadRig(const aiNode * aArmature)
{
    Rig result{
        .mArmatureName = aArmature->mName.C_Str(),
    };
    NodePointerMap aiNodeToTreeNode;

    //
    // Load the NodeTree of joints, starting from the armature
    //

    // TODO Ad 2024/02/22: Is it correct to start from the armature?
    std::tie(result.mJointTree, aiNodeToTreeNode) = loadJoints(aArmature);
    // I expect this to always be the case, the assert is here to catch if it becomes wrong.
    assert(result.mJointTree.mFirstRoot == 0);

    //auto maxVal = 
    //    *std::max_element(jointData.mVertexJointNumber.begin(), jointData.mVertexJointNumber.end());
    //std::cout << "Maximum number of bones influencing a single vertex: " << val << std::endl;

    return {result, aiNodeToTreeNode}; 
}

VertexJointData::BoneIndex_t JointDataDeduplicate::insertOrRetrieveBoneIndex(
    Rig::JointData & aOutJointData,
    NodeTree<Rig::Pose>::Node::Index aNodeIndex,
    math::AffineMatrix<4, float> aInverseBindMatrix)
{
    auto [iterator, didInsert] = 
        mUsedNodes.try_emplace(aNodeIndex, (VertexJointData::BoneIndex_t)aOutJointData.mIndices.size());
    auto jointDataIdx = iterator->second;
    if(!didInsert)
    {
        SELOG(warn)("Deduplicated a bone.");
        // When we deduplicate a bone, the IBMs must match
        assert(aOutJointData.mInverseBindMatrices[jointDataIdx] == aInverseBindMatrix);
    }
    else
    {
        aOutJointData.mIndices.push_back(aNodeIndex);
        aOutJointData.mInverseBindMatrices.push_back(std::move(aInverseBindMatrix));
    }
    return static_cast<VertexJointData::BoneIndex_t>(jointDataIdx);
}

//
// Iterate over all bones:
// * Populate the list of joints and their inverse bind matrices
// * Populate the joint data vertex attributes (array of VertexJointData)
//
std::vector<VertexJointData> populateJointData(JointDataDeduplicate & aDeduplicator,
                                               Rig::JointData & aOutJointData,
                                               const aiMesh * aMesh,
                                               const NodePointerMap & aAiNodeToTreeNode,
                                               const aiNode * aExpectedArmature)
{
    JointDataManager jointData{aMesh->mNumVertices};

    for(unsigned int boneIdx = 0; boneIdx != aMesh->mNumBones; ++boneIdx)
    {
        const aiBone & bone  = *aMesh->mBones[boneIdx];
        // I am making the assumption that the armature is a common root node of a skeleton
        // and thus it must be equal for all the bones.
        assert(bone.mArmature == aExpectedArmature);

        const VertexJointData::BoneIndex_t jointIndex =
            aDeduplicator.insertOrRetrieveBoneIndex(
                aOutJointData,
                aAiNodeToTreeNode.at(bone.mNode),
                math::AffineMatrix<4, float>{extractAffinePart(bone.mOffsetMatrix)});

        // Assert that the joint index fits in the destination type.
        assert(jointIndex < std::numeric_limits<VertexJointData::BoneIndex_t>::max());

        for(unsigned int weightIdx = 0; weightIdx != bone.mNumWeights; ++weightIdx)
        {
            const aiVertexWeight & weight = bone.mWeights[weightIdx];
            jointData.addJoint(weight.mVertexId, jointIndex, weight.mWeight);
        }
    }

    return jointData.mVertexJointData;
}


} // namespace ad::renderer