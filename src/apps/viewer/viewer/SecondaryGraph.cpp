#include "SecondaryGraph.h"


#include "DrawQuad.h"
#include "PassViewer.h"
#include "Scene.h"

#include "graph/Passes.h"

#include <snac-renderer-V2/Camera.h>
// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include <snac-renderer-V2/RendererReimplement.h>
#include <snac-renderer-V2/Profiling.h>
#include <snac-renderer-V2/Semantics.h>
#include <snac-renderer-V2/SetupDrawing.h>

#include <snac-renderer-V2/files/Loader.h>

#include <renderer/Uniforms.h>


namespace ad::renderer {


//
// SecondaryGraph
//
SecondaryGraph::SecondaryGraph(math::Size<2, int> aRenderSize,
                               const Loader & aLoader) :
    mRenderSize{aRenderSize}
{
    allocateSizeDependentTextures(mRenderSize);
    setupSizeDependentTextures();
}


void SecondaryGraph::resize(math::Size<2, int> aNewSize)
{
    // Note: for the moment, we keep the render size in sync with the framebuffer size
    // (this is not necessarily true in renderers where the rendered image is blitted to the default framebuffer)
    mRenderSize = aNewSize;

    // Re-initialize textures (because of immutable storage allocation)
    mDepthMap = graphics::Texture{GL_TEXTURE_2D};
    allocateSizeDependentTextures(mRenderSize);

    // Since the texture were re-initialized, they have to be setup
    setupSizeDependentTextures();
}


void SecondaryGraph::allocateSizeDependentTextures(math::Size<2, int> aSize)
{
    graphics::allocateStorage(mDepthMap, GL_DEPTH_COMPONENT24, aSize);
}


void SecondaryGraph::setupSizeDependentTextures()
{
    [this](GLenum aFiltering)
    {
        graphics::ScopedBind boundDepthMap{mDepthMap};
        // TODO take sampling and filtering out, it is about reading the texture, not writing it.
        glTexParameteri(mDepthMap.mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(mDepthMap.mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(mDepthMap.mTarget, 
                            GL_TEXTURE_BORDER_COLOR,
                            math::hdr::Rgba_f{1.f, 0.f, 0.f, 0.f}.data());

        glTexParameteri(mDepthMap.mTarget, GL_TEXTURE_MIN_FILTER, aFiltering);
        glTexParameteri(mDepthMap.mTarget, GL_TEXTURE_MAG_FILTER, aFiltering);
    }(GL_LINEAR/* seems required to get hardware pcf with sampler2DShadow */);

    //
    // Attaching a depth texture to a FBO
    //
    {
        graphics::ScopedBind boundFbo{mDepthFbo};
        // The attachment is permanent, no need to recreate it each time the FBO is bound
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthMap.mTarget, mDepthMap, /*mip map level*/0);

        assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
}


void SecondaryGraph::renderFrame(const Scene & aScene, 
                                 const ViewerPartList & aPartList,
                                 const Camera & aCamera,
                                 const LightsData & aLights_camera,
                                 const GraphShared & aGraphShared,
                                 Storage & aStorage,
                                 bool aShowTextures,
                                 const graphics::FrameBuffer & aFramebuffer)
{
    // Load the data that is per graph (actually, more per-view) in UBOs
    loadCameraUbo(*aGraphShared.mUbos.mViewingUbo, aCamera);
    loadLightsUbo(*aGraphShared.mUbos.mLightsUbo, aLights_camera);

    RepositoryTexture textureRepository;

    // Currently, the environment is the only *contextual* provider of a texture repo
    // (The other texture repo is part of the material)
    // TODO #repos This should be consolidated
    if(aScene.mEnvironment)
    {
        textureRepository = aScene.mEnvironment->mTextureRepository;
    }

    {
        graphics::ScopedBind boundFbo{mDepthFbo};
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());
        passOpaqueDepth(aGraphShared,  aPartList, textureRepository, aStorage);
    }

    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        // TODO Ad 2023/11/30: #perf Currently, this forward pass does not leverage the already available opaque depth-buffer
        // We cannot bind our "custom" depth buffer as the default framebuffer depth-buffer, so we will need to study alternatives:
        // render opaque depth to default FB, and blit to a texture for showing, render forward to a render target then blit to default FB, ...
        passForward(aGraphShared, aPartList, textureRepository, aStorage, mControls, false);
    }
}




} // namespace ad::renderer