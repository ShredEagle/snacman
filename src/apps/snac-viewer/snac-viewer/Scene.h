#pragma once

#include <snac-renderer/Camera.h>
#include <snac-renderer/Cube.h>

#include <graphics/AppInterface.h>

#include <math/VectorUtilities.h>


namespace ad {
namespace snac {


struct Scene
{
    Scene(graphics::AppInterface & aAppInterface);

    void update();

    Mesh mMesh{makeCube()};
    Camera mCamera;
    OrbitalControl mCameraControl;
};


inline Scene::Scene(graphics::AppInterface & aAppInterface) :
    mCamera{math::getRatio<float>(aAppInterface.getFramebufferSize())}
{
    using namespace std::placeholders;

    aAppInterface.registerCursorPositionCallback(
        std::bind(&OrbitalControl::callbackCursorPosition, &mCameraControl, _1, _2));

    aAppInterface.registerMouseButtonCallback(
        std::bind(&OrbitalControl::callbackMouseButton, &mCameraControl, _1, _2, _3, _4, _5));

    aAppInterface.registerScrollCallback(
        std::bind(&OrbitalControl::callbackScroll, &mCameraControl, _1, _2));
}


inline void Scene::update()
{
    mCamera.setWorldToCamera(mCameraControl.getParentToLocal());
}


} // namespace snac
} // namespace ad
