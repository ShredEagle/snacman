#pragma once


#include "CameraSystem.h"

#include <graphics/AppInterface.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Renderbuffer.h>

#include <memory>


namespace ad::renderer {


struct ViewerApplication;


struct SecondaryView
{
    SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface);

    // TODO: const the viewer app
    void render(/*const*/ ViewerApplication & aViewerApp);

    void blitIt(const graphics::FrameBuffer & aDestination);

    std::shared_ptr<graphics::AppInterface> mAppInterface;
    CameraSystem mCameraSystem;

    // TODO further split: FBOs are not shared, so they should be generated and bound on their specific contexts
    // (plus this will make this class more generic, usable even with single context)
    graphics::FrameBuffer mDrawFramebuffer;
    graphics::FrameBuffer mReadFramebuffer;
    graphics::Renderbuffer mColorBuffer;
    graphics::Renderbuffer mDepthBuffer;

    math::Size<2, int> mRenderSize;

    inline static constexpr GLint gColorAttachment = 0;
};


} // namespace ad::renderer