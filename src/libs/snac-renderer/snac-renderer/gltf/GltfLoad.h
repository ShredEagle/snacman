#pragma once


#include "../Mesh.h"

#include <platform/filesystem.h>


namespace ad {
namespace snac {


VertexStream loadGltf(filesystem::path aModel);



} // namespace snac
} // namespace ad
