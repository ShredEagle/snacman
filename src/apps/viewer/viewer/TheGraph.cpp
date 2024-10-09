#include "TheGraph.h"


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


namespace {


    void prepareShadowMap(const graphics::Texture & aShadowMap)
    {
        graphics::ScopedBind boundTexture{aShadowMap};
        gl.TexStorage3D(aShadowMap.mTarget,
                        1,
                        GL_DEPTH_COMPONENT24,
                        ShadowMapping::gTextureSize.width(),
                        ShadowMapping::gTextureSize.height(),
                        gMaxShadowLights);
        assert(graphics::isImmutableFormat(aShadowMap));

        // Set texture comparison mode, allowing to compare the depth component to a reference value
        glTexParameteri(aShadowMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(aShadowMap.mTarget, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glTexParameteri(aShadowMap.mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(aShadowMap.mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(aShadowMap.mTarget, 
                         GL_TEXTURE_BORDER_COLOR,
                         math::hdr::Rgba_f{1.f, 0.f, 0.f, 0.f}.data());
    }


    void setupShowTextureSampler(const graphics::Sampler & aSampler)
    {
        glSamplerParameteri(aSampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glObjectLabel(GL_SAMPLER, aSampler, -1, "show_texture_sampler");
    }


} // unnamed namespace


//
// TheGraph
//
TheGraph::TheGraph(math::Size<2, int> aRenderSize,
                   Storage & aStorage,
                   const Loader & aLoader) :
    mRenderSize{aRenderSize},
    mTransparencyResolver{aLoader.loadShader("shaders/TransparencyResolve.frag")},
    mShadowMap{makeTexture(aStorage, GL_TEXTURE_2D_ARRAY, "shadow_map")}
{
    allocateSizeDependentTextures(mRenderSize);
    setupSizeDependentTextures();

    prepareShadowMap(*mShadowMap);

    // Assign permanent texture units to glsl samplers used for the 2D transparency compositing.
    {
        graphics::setUniform(mTransparencyResolver.mProgram, "u_AccumTexture", gAccumTextureUnit);
        graphics::setUniform(mTransparencyResolver.mProgram, "u_RevealageTexture", gRevealageTextureUnit);
    }

    setupShowTextureSampler(mShowTextureSampler);
}


void TheGraph::resize(math::Size<2, int> aNewSize)
{
    // Note: for the moment, we keep the render size in sync with the framebuffer size
    // (this is not necessarily true in renderers where the rendered image is blitted to the default framebuffer)
    mRenderSize = aNewSize;

    // Re-initialize textures (because of immutable storage allocation)
    mDepthMap = graphics::Texture{GL_TEXTURE_2D};
    mTransparencyAccum = graphics::Texture{GL_TEXTURE_2D};
    mTransparencyRevealage = graphics::Texture{GL_TEXTURE_2D};

    allocateSizeDependentTextures(mRenderSize);

    // Since the texture were re-initialized, they have to be setup
    setupSizeDependentTextures();
}


void TheGraph::allocateSizeDependentTextures(math::Size<2, int> aSize)
{
    graphics::allocateStorage(mDepthMap, GL_DEPTH_COMPONENT24, aSize);
    graphics::allocateStorage(mTransparencyAccum, GL_RGBA16F, aSize);
    graphics::allocateStorage(mTransparencyRevealage, GL_R8, aSize);
}


void TheGraph::setupSizeDependentTextures()
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


void TheGraph::renderFrame(const Scene & aScene, 
                           const ViewerPartList & aPartList,
                           const Camera & aCamera,
                           const LightsData & aLights_camera,
                           const GraphShared & aGraphShared,
                           Storage & aStorage,
                           bool aShowTextures,
                           const graphics::FrameBuffer & aFramebuffer)
{
    RepositoryTexture textureRepository;

    // Shadow mapping
    LightViewProjection lightViewProjection = fillShadowMap(
        mShadowPass,
        textureRepository,
        aStorage,
        aGraphShared,
        aPartList,
        aScene.mRoot.mAabb,
        aCamera,
        mShadowMap,
        std::span{aScene.mLights_world.mDirectionalLights.data(), aScene.mLights_world.mDirectionalCount},
        aShowTextures /*debug draw*/ // Semantically incorrect, but we take it to also mean "we are the main view"
    );

    loadCameraUbo(*aGraphShared.mUbos.mViewingUbo, aCamera);
    loadLightsUbo(*aGraphShared.mUbos.mLightsUbo, aLights_camera);
    // For lights casting a shadow
    loadLightViewProjectionUbo(*aGraphShared.mUbos.mLightViewProjectionUbo, lightViewProjection);
    
    // Currently, the environment is the only *contextual* provider of a texture repo
    // (The other texture repo is part of the material)
    // TODO #repos This should be consolidated
    if(aScene.mEnvironment)
    {
        textureRepository = aScene.mEnvironment->mTextureRepository;
    }

    textureRepository[semantic::gShadowMap] = mShadowMap;

    {
        graphics::ScopedBind boundFbo{mDepthFbo};
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());
        passOpaqueDepth(aGraphShared, aPartList, textureRepository, aStorage);
    }

    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        // TODO Ad 2023/11/30: #perf Currently, this forward pass does not leverage the already available opaque depth-buffer
        // We cannot bind our "custom" depth buffer as the default framebuffer depth-buffer, so we will need to study alternatives:
        // render opaque depth to default FB, and blit to a texture for showing, render forward to a render target then blit to default FB, ...
        passForward(aGraphShared, aPartList, textureRepository, aStorage, mControls, aScene.mEnvironment.has_value());
    }

#if defined(ENABLED_TRANSPARENCY) // disable transparency while porting to "common graph"
    passTransparencyAccumulation(partList, textureRepository, aStorage);
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);
        passTransparencyResolve(partList, aStorage);
    }
#endif // ENABLED_TRANSPARENCY

    if(aScene.mEnvironment)
    {
        // The Framebuffer binding should be moved out of the passes, making them more reusable
        // Default Framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);
        glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        // When normally rendering the skybox, we are "inside" the cube, so only render backfaces
        passDrawSkybox(aGraphShared, *aScene.mEnvironment, aStorage, GL_FRONT, mControls);
    }

    //
    // Debug rendering of selected textures
    //
    if(aShowTextures)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aFramebuffer);

        auto [nearZ, farZ] = getNearFarPlanes(aCamera);
        showDepthTexture(mDepthMap, nearZ, farZ) ;
        //showTexture(mTransparencyAccum, 1, {.mOperation = DrawQuadParameters::AccumNormalize}) ;
        //showTexture(mTransparencyRevealage, 2, {.mSourceChannel = 0}) ;

        #define SHOW_SHADOWMAP

        #if defined(SHOW_ENV_TEXTURES)
        if(aScene.mEnvironment 
           && aScene.mEnvironment->mTextureRepository.count(semantic::gFilteredRadianceEnvironmentTexture) != 0)
        {
            showTexture(*aScene.mEnvironment->getEnvMap(),             1) ;
            showTexture(*aScene.mEnvironment->getFilteredRadiance(),   2) ;
            showTexture(*aScene.mEnvironment->getFilteredIrradiance(), 3) ;
        }
        #endif //SHOW_ENV_TEXTURES

        #if defined(SHOW_SHADOWMAP)
        showDepthTexture(*mShadowMap, -1, -10, 1);
        #endif //SHOW_SHADOWMAP 
    }
}


void TheGraph::showTexture(const graphics::Texture & aTexture,
                           unsigned int aStackPosition,
                           DrawQuadParameters aDrawParams)
{
    assert(aStackPosition < 4);

    // Unbind any texture, to avoid warning with samplers that would not be used
    // but are nonetheless declared in the drawquad fragment shader.
    glBindTextures(0, DrawQuadParameters::_EndSamplerType, NULL);

    // Note: with stencil, we could draw those rectangles first,
    // and prevent main rasterization behind them.
    
    switch(aTexture.mTarget)
    {
        default:
            aDrawParams.mSampler = DrawQuadParameters::Sampler2D;
            break;
        case GL_TEXTURE_2D_ARRAY:
            aDrawParams.mSampler = DrawQuadParameters::Sampler2DArray;
            break;
        case GL_TEXTURE_CUBE_MAP:
            aDrawParams.mSampler = DrawQuadParameters::CubeMap;
            break;
    }
    glActiveTexture(GL_TEXTURE0 + aDrawParams.mSampler);
    glBindTexture(aTexture.mTarget, aTexture);

    // Force custom sampler parameters, notably allowing to control GL_TEXTURE_COMPARE_MODE.
    // Note: glBindSampler expects the **zero-based** texture unit, not an enum starting form GL_TEXTURE0.
    graphics::ScopedBind boundSampler{mShowTextureSampler, aDrawParams.mSampler};

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


#if defined(ENABLED_TRANSPARENCY) // disable transparency while porting to "common graph"
void TheGraph::passTransparencyAccumulation(const ViewerPartList & aPartList,
                                            const RepositoryTexture & aTextureRepository,
                                            Storage & aStorage)
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
    ViewerPassCache passCache = prepareViewerPass(annotations, aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(passCache);

    // Draw
    {
        glViewport(0, 0,
                   mRenderSize.width(), mRenderSize.height());

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, aTextureRepository);
        }
    }
}


void TheGraph::passTransparencyResolve(const ViewerPartList & aPartList,
                                       Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_transparency_resolve", CpuTime, GpuTime);

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
#endif // disable transparency


} // namespace ad::renderer