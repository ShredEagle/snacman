#pragma once

#include <snac-renderer/Camera.h>
#include <snac-renderer/Cube.h>


namespace ad {
namespace snac {


struct Scene
{
    Mesh mMesh{makeCube()};
    Camera mCamera;
};


} // namespace snac
} // namespace ad
