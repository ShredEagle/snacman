#pragma once

#include <graphics/ApplicationGlfw.h>
#include <graphics/AppInterface.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer/Camera.h>
#include <snac-renderer/DebugRenderer.h>
#include <snac-renderer/Render.h>


namespace ad {
namespace snac {


struct Scene
{
    Scene(graphics::ApplicationGlfw & aGlfwApp,
          DebugRenderer aDebugRenderer,
          const resource::ResourceFinder & aFinder);

    void update();

    void render(Renderer & aRenderer);

    graphics::AppInterface & mAppInterface;
    // TODO #camera Address this duplication of Camera/CameraBuffer
    Camera mCamera;
    CameraBuffer mCameraBuffer;
    MouseOrbitalControl mCameraControl;
    const resource::ResourceFinder & mFinder;
    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListening;
    snac::DebugRenderer mDebugRenderer;
};


} // namespace snac
} // namespace ad