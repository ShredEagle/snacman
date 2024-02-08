#pragma once


#include "../Rigging.h"

#include <arte/gltf/Gltf.h>


namespace ad {
namespace snac {


/// @return A pair with first being the tree, where parents are guaranteed to happen 
/// **before** their children.
/// Second is an array mapping the node index in the gltf (array index) 
/// to an index in the node tree (array value).
std::pair<NodeTree<arte::gltf::Node::Matrix>, std::vector<Node::Index>>
loadHierarchy(const arte::Gltf & aGltf, arte::gltf::Index<arte::gltf::Scene> aSceneIndex);


std::pair<Rig, std::unordered_map<std::string, NodeAnimation>>
loadSkeletalAnimation(const arte::Gltf & aGltf,
                      arte::gltf::Index<arte::gltf::Skin> aSkinIndex,
                      arte::gltf::Index<arte::gltf::Scene> aSceneIndex);


} // namespace snac
} // namespace ad
