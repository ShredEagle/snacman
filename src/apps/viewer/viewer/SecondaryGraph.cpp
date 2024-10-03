#include "SecondaryGraph.h"


#include "DrawQuad.h"
#include "PassViewer.h"
#include "Scene.h"

#include <snac-renderer-V2/Camera.h>
// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include <snac-renderer-V2/RendererReimplement.h>
#include <snac-renderer-V2/Profiling.h>
#include <snac-renderer-V2/Semantics.h>
#include <snac-renderer-V2/SetupDrawing.h>

#include <snac-renderer-V2/files/Loader.h>

#include <renderer/Uniforms.h>


namespace ad::renderer {


constexpr std::size_t gMaxDrawInstances = 2048;


namespace {

    void loadFrameUbo(const graphics::UniformBufferObject & aUbo)
    {
        GLfloat time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
        proto::loadSingle(aUbo, time, graphics::BufferHint::DynamicDraw);
    }


    void loadLightsUbo(const graphics::UniformBufferObject & aUbo, const LightsData & aLights)
    {
        proto::loadSingle(aUbo, aLights, graphics::BufferHint::DynamicDraw);
    }
} // unnamed namespace


//
// SecondaryGraph
//
SecondaryGraph::SecondaryGraph(math::Size<2, int> aRenderSize,
                   Storage & aStorage,
                   const Loader & aLoader) :
    mUbos{aStorage},
    mInstanceStream{makeInstanceStream(aStorage, gMaxDrawInstances)},
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


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
void SecondaryGraph::loadDrawBuffers(const ViewerPassCache & aPassCache) const
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_draw_buffers", CpuTime, BufferMemoryWritten);

    assert(aPassCache.mDrawInstances.size() <= gMaxDrawInstances);

    // TODO Ad 2023/11/23: this hardcoded access to first buffer view is ugly
    proto::load(*mInstanceStream.mVertexBufferViews.at(0).mGLBuffer,
                std::span{aPassCache.mDrawInstances},
                graphics::BufferHint::DynamicDraw);

    proto::load(mIndirectBuffer,
                std::span{aPassCache.mDrawCommands},
                graphics::BufferHint::DynamicDraw);
}


void SecondaryGraph::renderFrame(const Scene & aScene, 
                           const Camera & aCamera,
                           const LightsData & aLights_camera,
                           Storage & aStorage,
                           bool aShowTextures,
                           const graphics::FrameBuffer & aFramebuffer)
{
    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    // Partial answer: the program selection is done later in preparePass (does not address camera overrides though)
    ViewerPartList partList = aScene.populatePartList();

    // Use the same indirect buffer for all drawings
    // Its content will be rewritten by distinct passes though
    // With the current approach, this binding could be done only once at construction,
    // unless several "Graph" instances are allowed
    graphics::bind(mIndirectBuffer, graphics::BufferType::DrawIndirect);

    RepositoryTexture textureRepository;

    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_frame_UBOs", CpuTime, GpuTime, BufferMemoryWritten);
        loadFrameUbo(*mUbos.mFrameUbo);
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, aCamera);

        loadLightsUbo(*mUbos.mLightsUbo, aLights_camera);

        // Load model transforms
        // Note: the model matrix transforms to world space, so it is independant of viewpoint.
        // (the pass indexes the transform for the objects it actually draws, so culling could still be implemented on top).
        proto::load(*mUbos.mModelTransformUbo,
                    std::span{partList.mInstanceTransforms},
                    graphics::BufferHint::DynamicDraw);

        // For lights casting a shadow
        //loadLightViewProjectionUbo(*mUbos.mLightViewProjectionUbo, lightViewProjection);


        proto::load(*mUbos.mJointMatrixPaletteUbo, std::span{partList.mRiggingPalettes}, graphics::BufferHint::StreamDraw);
    }

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
        passOpaqueDepth(partList, textureRepository, aStorage);
    }

    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO Ad 2023/11/30: #perf Currently, this forward pass does not leverage the already available opaque depth-buffer
        // We cannot bind our "custom" depth buffer as the default framebuffer depth-buffer, so we will need to study alternatives:
        // render opaque depth to default FB, and blit to a texture for showing, render forward to a render target then blit to default FB, ...
        passForward(partList, textureRepository, aStorage, false);
    }
}


void SecondaryGraph::passOpaqueDepth(const ViewerPartList & aPartList,
                               const RepositoryTexture & aTextureRepository,
                               Storage & aStorage) const
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_depth", CpuTime, GpuTime);

    auto annotations = selectPass("depth_opaque");
    // Can be done once even for distinct cameras, if there is no culling
    ViewerPassCache passCache = prepareViewerPass(annotations , aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(passCache);

    //
    // Set pipeline state
    //
    // Note: Must be set before any drawing operations, including glClear(),
    // otherwise results becomes real mysterious real quick.
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

    }

    // Clear must appear after the Framebuffer setup!
    gl.Clear(GL_DEPTH_BUFFER_BIT);

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, aTextureRepository);
        }
    }
}


void SecondaryGraph::passForward(const ViewerPartList & aPartList,
                           const RepositoryTexture & aTextureRepository,
                           Storage & aStorage,
                           bool aEnvironmentMapping)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_forward", CpuTime, GpuTime);

    // Can be done once for distinct camera, if there is no culling
    std::vector<Technique::Annotation> annotations{
        {"pass", *mControls.mForwardPassKey},
    };
    if(aEnvironmentMapping)
    {
        annotations.push_back({"environment", "on"});
    }
    ViewerPassCache passCache = prepareViewerPass(annotations, aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(passCache);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        // TODO handle pipeline state with an abstraction
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // We implemented alpha testing (cut-out), no blending.
        glDisable(GL_BLEND);

    }
    auto scopedPolygonMode = graphics::scopePolygonMode(*mControls.mForwardPolygonMode);

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        // TODO should be done once per viewport
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        // Reset texture bindings (to make sure that texturing does not work by "accident")
        // this could be extended to reset everything in the context.
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, aTextureRepository);
        }
    }
}


} // namespace ad::renderer