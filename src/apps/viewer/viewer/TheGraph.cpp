#include "TheGraph.h"


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


void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
{
    proto::loadSingle(aUbo, GpuViewProjectionBlock{aCamera}, graphics::BufferHint::DynamicDraw);
}


//
// HardcodedUbos
//
HardcodedUbos::HardcodedUbos(Storage & aStorage)
{
    aStorage.mUbos.emplace_back();
    mFrameUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gFrame, mFrameUbo);

    aStorage.mUbos.emplace_back();
    mViewingUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gViewProjection, mViewingUbo);

    aStorage.mUbos.emplace_back();
    mModelTransformUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLocalToWorld, mModelTransformUbo);

    aStorage.mUbos.emplace_back();
    mJointMatrixPaletteUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gJointMatrices, mJointMatrixPaletteUbo);

    aStorage.mUbos.emplace_back();
    mLightsUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLights, mLightsUbo);
}


//
// TheGraph
//
TheGraph::TheGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                   Storage & aStorage,
                   const Loader & aLoader) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    mUbos{aStorage},
    mInstanceStream{makeInstanceStream(aStorage, gMaxDrawInstances)},
    mFramebufferSizeListener{mGlfwAppInterface->listenFramebufferResize(
        std::bind(&TheGraph::onFramebufferResize, this, std::placeholders::_1))
    },
    mRenderSize{mGlfwAppInterface->getFramebufferSize()},
    mTransparencyResolver{aLoader.loadShader("shaders/TransparencyResolve.frag")},
    mSkybox{aLoader, aStorage},
    mDebugRenderer{aStorage, aLoader}
{
    allocateTextures(mRenderSize);
    setupTextures();

    // Assign permanent texture units to glsl samplers used for the 2D transparency compositing.
    {
        graphics::setUniform(mTransparencyResolver.mProgram, "u_AccumTexture", gAccumTextureUnit);
        graphics::setUniform(mTransparencyResolver.mProgram, "u_RevealageTexture", gRevealageTextureUnit);
    }
}


void TheGraph::onFramebufferResize(math::Size<2, int> aNewSize)
{
    // Note: for the moment, we keep the render size in sync with the framebuffer size
    // (this is not necessarily true in renderers where the rendered image is blitted to the default framebuffer)
    mRenderSize = aNewSize;

    // Re-initialize textures (because of immutable storage allocation)
    mDepthMap = graphics::Texture{GL_TEXTURE_2D};
    mTransparencyAccum = graphics::Texture{GL_TEXTURE_2D};
    mTransparencyRevealage = graphics::Texture{GL_TEXTURE_2D};

    allocateTextures(mRenderSize);

    // Since the texture were re-initialized, they have to be setup
    setupTextures();
}


void TheGraph::allocateTextures(math::Size<2, int> aSize)
{
    graphics::allocateStorage(mDepthMap, GL_DEPTH_COMPONENT24, aSize);
    graphics::allocateStorage(mTransparencyAccum, GL_RGBA16F, aSize);
    graphics::allocateStorage(mTransparencyRevealage, GL_R8, aSize);
}


void TheGraph::setupTextures()
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

    //
    // Setup transparency framebuffer
    //
    {
        graphics::ScopedBind boundFbo{mTransparencyFbo};
        // Reuse existing opaque depth buffer
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthMap.mTarget, mDepthMap, /*mip map level*/0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTransparencyAccum.mTarget, mTransparencyAccum, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, mTransparencyRevealage.mTarget, mTransparencyRevealage, 0);

        const GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, buffers);

        assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
}


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
// TODO Ad 2023/11/26: Even more, since the model transform is to world space (i.e. constant accross viewpoints)
// and it is fetched from a buffer in the shaders, it could actually be pushed once per frame.
// (and the pass would only index the transform for the objects it actually draws).
void TheGraph::loadDrawBuffers(const ViewerPartList & aPartList,
                               const ViewerPassCache & aPassCache)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_draw_buffers", CpuTime, BufferMemoryWritten);

    assert(aPassCache.mDrawInstances.size() <= gMaxDrawInstances);

    proto::load(*mUbos.mModelTransformUbo,
                std::span{aPartList.mInstanceTransforms},
                graphics::BufferHint::DynamicDraw);

    // TODO Ad 2023/11/23: this hardcoded access to first buffer view is ugly
    proto::load(*mInstanceStream.mVertexBufferViews.at(0).mGLBuffer,
                std::span{aPassCache.mDrawInstances},
                graphics::BufferHint::DynamicDraw);

    proto::load(mIndirectBuffer,
                std::span{aPassCache.mDrawCommands},
                graphics::BufferHint::DynamicDraw);
}


void TheGraph::renderFrame(const Scene & aScene, 
                           const Camera & aCamera,
                           const LightsData & aLights_camera,
                           Storage & aStorage)
{
    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    // Partial answer: the program selection is done later in preparePass (does not address camera overrides though)
    ViewerPartList partList = aScene.populatePartList();

    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_frame_UBOs", CpuTime, GpuTime, BufferMemoryWritten);
        loadFrameUbo(*mUbos.mFrameUbo);
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, aCamera);

        loadLightsUbo(*mUbos.mLightsUbo, aLights_camera);

        proto::load(*mUbos.mJointMatrixPaletteUbo, std::span{partList.mRiggingPalettes}, graphics::BufferHint::StreamDraw);
    }

    // Use the same indirect buffer for all drawings
    // Its content will be rewritten by distinct passes though
    // With the current approach, this binding could be done only once at construction
    graphics::bind(mIndirectBuffer, graphics::BufferType::DrawIndirect);

    passOpaqueDepth(partList, aStorage);
    // TODO Ad 2023/11/30: #perf Currently, this forward pass does not leverage the already available opaque depth-buffer
    // We cannot bind our "custom" depth buffer as the default framebuffer depth-buffer, so we will need to study alternatives:
    // render opaque depth to default FB, and blit to a texture for showing, render forward to a render target then blit to default FB, ...
    passForward(partList, aStorage, aScene.mEnvironment.has_value());
    passTransparencyAccumulation(partList, aStorage);
    passTransparencyResolve(partList, aStorage);

    if(aScene.mEnvironment)
    {
        // The Framebuffer binding should be moved out of the passes, making them more reusable
        // Default Framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        passSkybox(*aScene.mEnvironment, aStorage);
    }

    //
    // Debug rendering of the depth texture
    //
    auto [nearZ, farZ] = getNearFarPlanes(aCamera);
    showDepthTexture(mDepthMap, nearZ, farZ) ;
    showTexture(mTransparencyAccum, 1, {.mOperation = DrawQuadParameters::AccumNormalize}) ;
    showTexture(mTransparencyRevealage, 2, {.mSourceChannel = 0}) ;
}


void TheGraph::passSkybox(const Environment & aEnvironment, Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_skybox", CpuTime, GpuTime);

    Handle<const graphics::Texture> envMap = aEnvironment.mMap;

    glBindVertexArray(*mSkybox.mCubeVao);
    glActiveTexture(GL_TEXTURE5);
    switch(aEnvironment.mType)
    {
        case Environment::Cubemap:
            glUseProgram(mSkybox.mCubemapProgram);
            glBindTexture(GL_TEXTURE_CUBE_MAP, *envMap);
            break;
        case Environment::Equirectangular:
            glUseProgram(mSkybox.mEquirectangularProgram);
            glBindTexture(GL_TEXTURE_2D, *envMap);
            break;
    }

    graphics::setUniform(mSkybox.mCubemapProgram, "u_SkyboxTexture", 5);
    graphics::setUniform(mSkybox.mEquirectangularProgram, "u_SkyboxTexture", 5);

    // This is bad, harcoding knowledge of the binding points
    // but this was written as a refresher on native GL
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, *mUbos.mViewingUbo);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    auto scopedPolygonMode = graphics::scopePolygonMode(*mControls.mForwardPolygonMode);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glCullFace(GL_BACK);
}


void TheGraph::renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage)
{
    // Fullscreen viewport
    glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

    mDebugRenderer.render(std::move(aDrawList), mUbos.mUboRepository, aStorage);
}


void TheGraph::showTexture(const graphics::Texture & aTexture,
                           unsigned int aStackPosition,
                           DrawQuadParameters aDrawParams)
{
    assert(aStackPosition < 4);

    // Note: with stencil, we could draw those rectangles first,
    // and prevent main rasterization behind them.

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(aTexture.mTarget, aTexture);

    // Would require scissor test to clear only part of the screen.
    //gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDepthMask(GL_FALSE);
    }

    math::Size<2, int> fbSize = mRenderSize;

    glViewport(0,
               aStackPosition * fbSize.height() / 4,
               fbSize.width() / 4,
               fbSize.height() / 4);

    drawQuad(aDrawParams);
}


// TODO rewrite in terms of above
void TheGraph::showDepthTexture(const graphics::Texture & aTexture,
                                float aNearZ, float aFarZ,
                                unsigned int aStackPosition)
{
    showTexture(
        aTexture, 
        aStackPosition,
        {
            .mSourceChannel = 0,
            .mOperation = DrawQuadParameters::DepthLinearize1,
            .mNearDistance = aNearZ,
            .mFarDistance = aFarZ,
        });
}


void TheGraph::passOpaqueDepth(const ViewerPartList & aPartList, Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_depth", CpuTime, GpuTime);

    auto annotations = selectPass("depth_opaque");
    // Can be done once even for distinct cameras, if there is no culling
    ViewerPassCache passCache = prepareViewerPass(annotations , aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aPartList, passCache);


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

        // Viewport is coupled to the depth map here
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());
    }

    graphics::ScopedBind boundFbo{mDepthFbo};

    // Clear must appear after the Framebuffer setup!
    gl.Clear(GL_DEPTH_BUFFER_BIT);

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, mTextureRepository);
        }
    }
}


void TheGraph::passForward(const ViewerPartList & aPartList,
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
        annotations.emplace_back("environment", "on");
    }
    ViewerPassCache passCache = prepareViewerPass(annotations, aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aPartList, passCache);

    // Default Framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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

    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
            draw(passCache, mUbos.mUboRepository, mTextureRepository);
        }
    }
}


void TheGraph::passTransparencyAccumulation(const ViewerPartList & aPartList, Storage & mStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_transparency_accum", CpuTime, GpuTime);

    //
    // Clear output resources
    //
    {
        static const std::array<GLhalf, 4> accumClearColor{0, 0, 0, 0};
        glClearTexImage(mTransparencyAccum, 0, GL_RGBA, GL_HALF_FLOAT, accumClearColor.data());
    }

    {
        GLubyte revealageClearColor = std::numeric_limits<GLubyte>::max();
        glClearTexImage(mTransparencyRevealage, 0, GL_RED, GL_UNSIGNED_BYTE, &revealageClearColor);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mTransparencyFbo);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        // TODO handle pipeline state with an abstraction
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST); // Discard transparent fragments behind the closest opaque fragment.
        glDepthMask(GL_FALSE); // Do not update the depth buffer, so we can render transparents in any order.

        // Setup the blending as prescribed
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        // Accum
        glBlendFunci(0, GL_ONE, GL_ONE);
        // Revealage
        glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    }

    // Can be done once for distinct camera, if there is no culling
    auto annotations = selectPass("transparent");
    ViewerPassCache passCache = prepareViewerPass(annotations, aPartList, mStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aPartList, passCache);

    // Draw
    {
        glViewport(0, 0,
                   mRenderSize.width(), mRenderSize.height());

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, mTextureRepository);
        }
    }
}


void TheGraph::passTransparencyResolve(const ViewerPartList & aPartList, Storage & mStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_transparency_resolve", CpuTime, GpuTime);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        // TODO handle pipeline state with an abstraction
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        // Setup the blending as prescribed
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        // Accum
        glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Bind textures to textures units

    // This is a manual approach, not aligned with our usual dynamic "IntrospectProgram" appraoch
    // This has the advantage that it should do less driver work than the dynamic version,
    // but this decision should be reviewed.
    glActiveTexture(GL_TEXTURE0 + gAccumTextureUnit);
    bind(mTransparencyAccum);
    glActiveTexture(GL_TEXTURE0 + gRevealageTextureUnit);
    bind(mTransparencyRevealage);

    // Draw
    {
        glViewport(0, 0,
                   mRenderSize.width(), mRenderSize.height());

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_quad", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            mTransparencyResolver.drawQuad();
        }
    }
}


} // namespace ad::renderer