#pragma once


#include <snac-renderer-V2/Rigging.h>

#include <unordered_map>


struct aiMesh;
struct aiNode;


namespace ad::renderer {


//NodeTree<Rig::Pose> loadJoints(aiMesh * aMesh);

using NodePointerMap = std::unordered_map<const aiNode *, NodeTree<Rig::Pose>::Node::Index>;

std::pair<Rig, NodePointerMap> loadRig(const aiNode * aArmature);

std::vector<VertexJointData> populateJointData(Rig::JointData & aOutJointData,
                                               const aiMesh * aMesh,
                                               const NodePointerMap & aAiNodeToTreeNode,
                                               const aiNode * aExpectedArmature);


} // namespace ad::renderer