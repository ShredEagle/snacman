#pragma once


#include "Mesh.h"

#include <platform/filesystem.h>

#include <memory>


namespace ad {
namespace snac {


std::shared_ptr<Effect> loadEffect(filesystem::path aProgram);


Mesh loadCube(std::shared_ptr<Effect> aEffect);


} // namespace graphics
} // namespace ad