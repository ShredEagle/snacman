#pragma once


#include "CameraSystem.h"
#include "TheGraph.h"

#include <graphics/AppInterface.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Renderbuffer.h>

#include <memory>


namespace ad::renderer {


struct ViewerApplication;

// TODO #graph remove
struct Loader;
struct Storage;

struct SecondaryView
{
    // Not easily copyable nor movable, due to the listener which binds to this.
    // (Note: moving the subobject to the heap might be an easy workaround)
    SecondaryView(const SecondaryView &) = delete;
    SecondaryView(SecondaryView &&) = delete;

    SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                  // TODO #graph get rid of those, when addressing the problematic design of TheGraph coupling much too many things
                  Storage & aStorage,
                  const Loader & aLoader);

    
    void resize(math::Size<2, int> aNewSize);

    // Private
    void allocateBuffers();

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
    TheGraph mGraph;
    std::shared_ptr<graphics::AppInterface::SizeListener> mFramebufferSizeListener;

    inline static constexpr GLint gColorAttachment = 0;
};


} // namespace ad::renderer