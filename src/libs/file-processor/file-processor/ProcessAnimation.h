#pragma once


#include <snac-renderer-V2/Rigging.h>

#include <unordered_map>


struct aiMesh;
struct aiNode;


namespace ad::renderer {


//NodeTree<Rig::Pose> loadJoints(aiMesh * aMesh);

using NodePointerMap = std::unordered_map<const aiNode *, NodeTree<Rig::Pose>::Node::Index>;

std::pair<Rig, NodePointerMap> loadRig(const aiNode * aArmature);


/// @brief Implement a cache used to deduplicate bones (bones that map to the same aiNode)
struct JointDataDeduplicate
{
    VertexJointData::BoneIndex_t insertOrRetrieveBoneIndex(
        Rig::JointData & aOutJointData,
        NodeTree<Rig::Pose>::Node::Index aNodeIndex,
        math::AffineMatrix<4, float> aInverseBindMatrix);

    // Maps the index of a NodeTree::Node, used as bone, to its position in the array of Node indices (Rig::JointData::mIndices)
    // This cache allows to deduplicate bones that are used by several meshes of the same Node
    // (we can reuse the Bone index that was assigned the first time the node was encountered, instead of assigning another).
    std::unordered_map<NodeTree<Rig::Pose>::Node::Index, /*Node index*/
                       std::vector<NodeTree<Rig::Pose>::Node::Index>::size_type /* Bone index */> mUsedNodes;
};


std::vector<VertexJointData> populateJointData(JointDataDeduplicate & aDeduplicator,
                                               Rig::JointData & aOutJointData,
                                               const aiMesh * aMesh,
                                               const NodePointerMap & aAiNodeToTreeNode,
                                               const aiNode * aExpectedArmature);


} // namespace ad::renderer