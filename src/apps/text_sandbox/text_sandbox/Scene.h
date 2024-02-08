#pragma once

#include <graphics/ApplicationGlfw.h>
#include <graphics/AppInterface.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer-V1/Camera.h>
#include <snac-renderer-V1/DebugRenderer.h>
#include <snac-renderer-V1/Render.h>


namespace ad {
namespace snac {


constexpr unsigned int gFontPixelHeight = 50;


struct Scene
{
    Scene(graphics::ApplicationGlfw & aGlfwApp,
          Font aFont,
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

    Font mFont;
    GlyphInstanceStream mGlyphs;
    TextRenderer mTextRenderer;
};


} // namespace snac
} // namespace ad