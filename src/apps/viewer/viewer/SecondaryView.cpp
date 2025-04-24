#include "SecondaryView.h"

#include "ViewerApplication.h"

#include <graphics/CameraUtilities.h>

#include <snac-renderer-V2/Profiling.h>
#include <snac-renderer-V2/Semantics.h>


namespace ad::renderer {


SecondaryView::SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                             const imguiui::ImguiUi & aImguiUi,
                             Storage & aStorage,
                             const Loader & aLoader) : 
    mAppInterface{std::move(aGlfwAppInterface)},
    mCameraSystem{
        mAppInterface,
        &aImguiUi,
        CameraSystem::Control::Orbital,
        Orbital{
            5.f,
            math::Degree<float>{0.f}
        }},
    mRenderSize{mAppInterface->getFramebufferSize()},
    mGraph{mRenderSize, aLoader},
    mFramebufferSizeListener{mAppInterface->listenFramebufferResize(
        std::bind(&SecondaryView::resize, this, std::placeholders::_1))}
{
    allocateBuffers();

    mCameraSystem.mCamera.setupOrthographicProjection(Camera::OrthographicParameters{
        .mAspectRatio = math::getRatio<float>(mRenderSize.as<math::Size, float>()),
        .mViewHeight = 2,
        .mNearZ = 0,
        .mFarZ = -200,
    });

    // Attach the render buffers to FBO
    graphics::ScopedBind boundFbo{mDrawFramebuffer, graphics::FrameBufferTarget::Draw};

    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0 + gColorAttachment,
                              GL_RENDERBUFFER,
                              mColorBuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + gColorAttachment);

    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              mDepthBuffer);
}


void SecondaryView::resize(math::Size<2, int> aNewSize)
{
    mRenderSize = aNewSize;
    allocateBuffers();
    mGraph.resize(mRenderSize);
}


void SecondaryView::allocateBuffers()
{
    {
        graphics::ScopedBind boundRenderbuffer{mColorBuffer};
        glRenderbufferStorage(
            GL_RENDERBUFFER, 
            GL_RGBA8,
            mRenderSize.width(), mRenderSize.height());
    }
    {
        graphics::ScopedBind boundRenderbuffer{mDepthBuffer};
        glRenderbufferStorage(
            GL_RENDERBUFFER, 
            GL_DEPTH_COMPONENT32,
            mRenderSize.width(), mRenderSize.height());
    }
}


void SecondaryView::update(const Timing & aTime)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "SecondaryView::update()", CpuTime);

    mCameraSystem.update(aTime.mDeltaDuration);
}


void SecondaryView::render(const Scene & aScene,
                           const ViewerPartList & aPartList,
                           bool aLightsInCameraSpace,
                           const GraphShared & aGraphShared,
                           Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "SecondaryView::render()", CpuTime, GpuTime, BufferMemoryWritten);

    // To remove
    //graphics::ScopedBind boundFbo{mDrawFramebuffer, graphics::FrameBufferTarget::Draw};
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mGraph.renderFrame(aScene,
                       aPartList,
                       mCameraSystem.mCamera,
                       aScene.getLightsInCamera(mCameraSystem.mCamera, !aLightsInCameraSpace),
                       aGraphShared,
                       aStorage,
                       false,
                       mDrawFramebuffer);
}


void ViewBlitter::blitIt(const graphics::Renderbuffer & aColorBuffer,
                         math::Rectangle<GLint> aViewport,
                         const graphics::FrameBuffer & aDestination)
{
    graphics::ScopedBind boundReadFbo{mReadFramebuffer, graphics::FrameBufferTarget::Read};
    //graphics::ScopedBind boundRenderbuffer{aColorBuffer};
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              aColorBuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // We expect the size of the read renderbuffer to exactly match that of the output draw buffer.
    // This is the reponsibility of the client
    graphics::ScopedBind boundDrawFbo{aDestination, graphics::FrameBufferTarget::Draw};
    //glDrawBuffer(GL_BACK);
    
    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBlitFramebuffer(aViewport.xMin(), aViewport.yMin(), aViewport.xMax(), aViewport.yMax(),
                      aViewport.xMin(), aViewport.yMin(), aViewport.xMax(), aViewport.yMax(),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
}


} // namespace ad::renderer