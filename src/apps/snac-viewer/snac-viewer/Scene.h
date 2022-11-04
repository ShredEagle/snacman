#pragma once

#include <snac-renderer/Cube.h>


namespace ad {
namespace snac {


struct Scene
{
    Mesh mMesh{makeCube()};
};


} // namespace snac
} // namespace ad
