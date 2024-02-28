#pragma once

#include <snac-renderer-V2/Rigging.h>


struct aiMesh;


namespace ad::renderer {


//NodeTree<Rig::Pose> loadJoints(aiMesh * aMesh);

std::pair<Rig, VertexJointData> loadRig(const aiMesh * aMesh);


} // namespace ad::renderer