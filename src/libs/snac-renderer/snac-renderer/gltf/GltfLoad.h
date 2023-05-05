#pragma once


#include "../Mesh.h"
#include "../Rigging.h"

#include <arte/gltf/Gltf.h>

#include <platform/Filesystem.h>


namespace ad {
namespace snac {


Model loadGltf(const arte::Gltf & aGltf, std::string_view aName);

inline Model loadGltf(filesystem::path aModel, std::string_view aName)
{
    return loadGltf(arte::Gltf{aModel}, aName);
}


inline Model loadGltf(filesystem::path aModel)
{
    // TODO would be more correct to actually use the full identifier to the model
    // as requested by the "top level" code.
    // Sadly, at this point this information is lost, we only have the actual full path.
    return loadGltf(aModel, aModel.filename().string());
}

/// @return A pair with first being the tree, where parents are guaranteed to happen 
/// **before** their children.
/// Second is an array mapping the node index in the gltf (array index) 
/// to an index in the node tree (array value).
std::pair<NodeTree<arte::gltf::Node::Matrix>, std::vector<Node::Index>>
loadHierarchy(const arte::Gltf & aGltf, std::size_t aSceneIndex = 0);


} // namespace snac
} // namespace ad
