#pragma once


#include "Mesh.h"

#include <platform/Filesystem.h>

#include <memory>


namespace ad {
namespace snac {


std::shared_ptr<Effect> loadEffect(filesystem::path aProgram);


Mesh loadCube(std::shared_ptr<Effect> aEffect);

Mesh loadModel(filesystem::path aGltf, std::shared_ptr<Effect> aEffect);


} // namespace graphics
} // namespace ad