#include "SecondaryView.h"

#include "ViewerApplication.h"

#include <graphics/CameraUtilities.h>

#include <snac-renderer-V2/Profiling.h>


namespace ad::renderer {


SecondaryView::SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface) :
    mAppInterface{std::move(aGlfwAppInterface)},
    mCameraSystem{mAppInterface, nullptr /* no inhibiter*/, CameraSystem::Control::FirstPerson},
    mRenderSize{mAppInterface->getFramebufferSize()}
{
    mCameraSystem.mCamera.setupOrthographicProjection(Camera::OrthographicParameters{
        .mAspectRatio = math::getRatio<float>(mRenderSize.as<math::Size, float>()),
        .mViewHeight = 2,
        .mNearZ = 0,
        .mFarZ = -100,
    });
    mCameraSystem.mCamera.setPose(
        graphics::getCameraTransform<GLfloat>({0.f, 5.f, 0.f},
                                              {0.f, -1.f, 0.f},
                                              {0.f, 0.f, -1.f}));
}

void SecondaryView::render(ViewerApplication & aViewerApp)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "SecondaryView::render()", CpuTime, GpuTime, BufferMemoryWritten);

    // TODO handle resizing

    // TODO handle initializing only once...
    graphics::ScopedBind boundFbo{mDrawFramebuffer, graphics::FrameBufferTarget::Draw};

    {
        graphics::ScopedBind boundRenderbuffer{mColorBuffer};
        glRenderbufferStorage(
            GL_RENDERBUFFER, 
            GL_RGBA8,
            mRenderSize.width(), mRenderSize.height());
    }
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0 + gColorAttachment,
                            GL_RENDERBUFFER,
                            mColorBuffer);

    {
        graphics::ScopedBind boundRenderbuffer{mDepthBuffer};
        glRenderbufferStorage(
            GL_RENDERBUFFER, 
            GL_DEPTH_COMPONENT32,
            mRenderSize.width(), mRenderSize.height());
    }
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER,
                            mDepthBuffer);


    // TODO clear the buffers!
    aViewerApp.mGraph.renderFrame(aViewerApp.mScene,
                                  mCameraSystem.mCamera,
                                  aViewerApp.mScene.getLightsInCamera(mCameraSystem.mCamera,
                                                                      !aViewerApp.mSceneGui.getOptions().mAreDirectionalLightsCameraSpace),
                                  aViewerApp.mStorage,
                                  false,
                                  mDrawFramebuffer);
                                  //false);
}


void SecondaryView::blitIt(const graphics::FrameBuffer & aDestination)
{
    graphics::ScopedBind boundReadFbo{mReadFramebuffer, graphics::FrameBufferTarget::Read};
    graphics::ScopedBind boundRenderbuffer{mColorBuffer};
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0 + gColorAttachment,
                              GL_RENDERBUFFER,
                              mColorBuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + gColorAttachment);

    // We expect the size of the read renderbuffer to exactly match that of the output draw buffer.
    graphics::ScopedBind boundDrawFbo{aDestination, graphics::FrameBufferTarget::Draw};
    //glDrawBuffer(GL_BACK);
    
    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBlitFramebuffer(0, 0, mRenderSize.width(), mRenderSize.height(),
                      0, 0, mRenderSize.width(), mRenderSize.height(),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
}


} // namespace ad::renderer