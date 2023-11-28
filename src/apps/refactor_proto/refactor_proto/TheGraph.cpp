#include "TheGraph.h"

// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include "RendererReimplement.h" 

#include "Camera.h"
#include "DrawQuad.h"
#include "Loader.h" // for semantics
#include "Pass.h"
#include "Profiling.h"
#include "Scene.h"
#include "SetupDrawing.h"



namespace ad::renderer {


constexpr std::size_t gMaxDrawInstances = 2048;


namespace {


    /// @brief Layout compatible with the shader's `ViewBlock`
    struct GpuViewBlock
    {
        GpuViewBlock(
            math::AffineMatrix<4, GLfloat> aWorldToCamera,
            math::Matrix<4, 4, GLfloat> aProjection 
        ) :
            mWorldToCamera{aWorldToCamera},
            mProjection{aProjection},
            mViewingProjection{aWorldToCamera * aProjection}
        {}

        GpuViewBlock(const Camera & aCamera) :
            GpuViewBlock{aCamera.getParentToCamera(), aCamera.getProjection()}
        {}

        math::AffineMatrix<4, GLfloat> mWorldToCamera; 
        math::Matrix<4, 4, GLfloat> mProjection; 
        math::Matrix<4, 4, GLfloat> mViewingProjection;
    };


    void loadFrameUbo(const graphics::UniformBufferObject & aUbo)
    {
        GLfloat time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
        proto::loadSingle(aUbo, time, graphics::BufferHint::DynamicDraw);
    }


    void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
    {
        proto::loadSingle(aUbo, GpuViewBlock{aCamera}, graphics::BufferHint::DynamicDraw);
    }


} // unnamed namespace


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
    mUboRepository.emplace(semantic::gView, mViewingUbo);

    aStorage.mUbos.emplace_back();
    mModelTransformUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLocalToWorld, mModelTransformUbo);
}


//
// Draw
//
void draw(const PassCache & aPassCache,
          const RepositoryUbo & aUboRepository,
          const RepositoryTexture & aTextureRepository)
{
    //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
    // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
    // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
    // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
    // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

    GLuint firstInstance = 0; 
    for (const DrawCall & aCall : aPassCache.mCalls)
    {
        PROFILER_SCOPE_RECURRING_SECTION("drawcall_iteration", CpuTime);

        PROFILER_PUSH_RECURRING_SECTION("discard_2", CpuTime);
        const IntrospectProgram & selectedProgram = *aCall.mProgram;
        const graphics::VertexArrayObject & vao = *aCall.mVao;
        PROFILER_POP_RECURRING_SECTION;

        // TODO Ad 2023/10/05: #perf #azdo 
        // Only change what is necessary, instead of rebiding everything each time.
        // Since we sorted our draw calls, it is very likely that program remain the same, and VAO changes.
        {
            PROFILER_PUSH_RECURRING_SECTION("bind_VAO", CpuTime);
            graphics::ScopedBind vaoScope{vao};
            PROFILER_POP_RECURRING_SECTION;
            
            {
                PROFILER_SCOPE_RECURRING_SECTION("set_buffer_backed_blocks", CpuTime);
                // TODO #repos This should be consolidated
                RepositoryUbo uboRepo{aUboRepository};
                if(aCall.mCallContext)
                {
                    RepositoryUbo callRepo{aCall.mCallContext->mUboRepo};
                    uboRepo.merge(callRepo);
                }
                setBufferBackedBlocks(selectedProgram, uboRepo);
            }

            {
                PROFILER_SCOPE_RECURRING_SECTION("set_textures", CpuTime);
                // TODO #repos This should be consolidated
                RepositoryTexture textureRepo{aTextureRepository};
                if(aCall.mCallContext)
                {
                    RepositoryTexture callRepo{aCall.mCallContext->mTextureRepo};
                    textureRepo.merge(callRepo);
                }
                setTextures(selectedProgram, textureRepo);
            }

            PROFILER_PUSH_RECURRING_SECTION("bind_program", CpuTime);
            graphics::ScopedBind programScope{selectedProgram};
            PROFILER_POP_RECURRING_SECTION;

            {
                // TODO Ad 2023/08/23: Measuring GPU time here has a x2 impact on cpu performance
                // Can we have efficient GPU measures?
                PROFILER_SCOPE_RECURRING_SECTION("glDraw_call", CpuTime/*, GpuTime*/);
                
                gl.MultiDrawElementsIndirect(
                    aCall.mPrimitiveMode,
                    aCall.mIndicesType,
                    (void *)(firstInstance * sizeof(DrawElementsIndirectCommand)),
                    aCall.mPartCount,
                    sizeof(DrawElementsIndirectCommand));
            }
        }
        firstInstance += aCall.mPartCount;
    }
}


//
// TheGraph
//
TheGraph::TheGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                   Storage & aStorage) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    mUbos{aStorage},
    mInstanceStream{makeInstanceStream(aStorage, gMaxDrawInstances)}
{
    // TODO Ad 2023/11/28: Listen to framebuffer resize
    mDepthMapSize = mGlfwAppInterface->getFramebufferSize();

    graphics::allocateStorage(mDepthMap, GL_DEPTH_COMPONENT24, mDepthMapSize);
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
    }
}


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
// TODO Ad 2023/11/26: Even more, since the model transform is to world space (i.e. constant accross viewpoints)
// and it is fetched from a buffer in the shaders, it could actually be pushed once per frame.
// (and the pass would only index the transform for the objects it actually draws).
void TheGraph::loadDrawBuffers(const PartList & aPartList,
                               const PassCache & aPassCache)
{
    PROFILER_SCOPE_RECURRING_SECTION("load_draw_buffers", CpuTime, BufferMemoryWritten);

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


void TheGraph::renderFrame(const PartList & aPartList, const Camera & aCamera, Storage & aStorage)
{
    {
        PROFILER_SCOPE_RECURRING_SECTION("load_frame_UBOs", CpuTime, GpuTime, BufferMemoryWritten);
        loadFrameUbo(*mUbos.mFrameUbo);
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, aCamera);
    }

    // Use the same indirect buffer for all drawings
    // Its content will be rewritten by distinct passes though
    // With the current approach, this binding could be done only once at construction
    graphics::bind(mIndirectBuffer, graphics::BufferType::DrawIndirect);

    passDepth(aPartList, aStorage);
    passForward(aPartList, aStorage);

    //
    // Debug rendering of the depth texture
    //
    auto [nearZ, farZ] = getNearFarPlanes(aCamera);
    showDepthTexture(mDepthMap, nearZ, farZ) ;
}


void TheGraph::showDepthTexture(const graphics::Texture & aTexture,
                                float aNearZ, float aFarZ,
                                unsigned int aStackPosition)
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

    math::Size<2, int> fbSize = mGlfwAppInterface->getFramebufferSize();

    glViewport(0,
               aStackPosition * fbSize.height() / 4,
               fbSize.width() / 4,
               fbSize.height() / 4);

    drawQuad({
        .mSourceChannel = 0,
        .mLinearization = DrawQuadParameters::DepthMethod1,
        .mNearDistance = aNearZ,
        .mFarDistance = aFarZ,
    });
}

void TheGraph::passDepth(const PartList & aPartList, Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION("pass_depth", CpuTime, GpuTime);

    // Can be done once even for distinct cameras, if there is no culling
    PassCache passCache = preparePass("depth", aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aPartList, passCache);


    //
    // Set pipeline state
    //
    // Note: Must be set before any drawing operations, including glClear(),
    // otherwise results becomes real mysterious real quick.
    {
        PROFILER_SCOPE_RECURRING_SECTION("set_pipeline_state", CpuTime);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // Viewport is coupled to the depth map here
        glViewport(0, 0, mDepthMapSize.width(), mDepthMapSize.height());
    }

    graphics::ScopedBind boundFbo{mDepthFbo};

    // Check bound FBO status before rendering
    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // Clear must appear after the Framebuffer setup!
    gl.Clear(GL_DEPTH_BUFFER_BIT);

    ///// TODO remove when it is used in another pass
    //static graphics::Texture colorTex = [](math::Size<2, int> aTextureSize)
    //{
    //    graphics::Texture tex{GL_TEXTURE_2D};
    //    graphics::allocateStorage(tex, GL_RGBA8, aTextureSize);
    //    return tex;
    //}(depthMapSize);

    //glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTex.mTarget, colorTex, /*mip map level*/0);
    //const GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
    //glDrawBuffers(1, buffers);

    //assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    //gl.Clear(GL_COLOR_BUFFER_BIT);
    /////

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        {
            PROFILER_SCOPE_RECURRING_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, mDummyTextureRepository);
        }
    }
}


void TheGraph::passForward(const PartList & aPartList, Storage & mStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION("pass_forward", CpuTime, GpuTime);

    // Can be done once for distinct camera, if there is no culling
    PassCache passCache = preparePass("forward", aPartList, mStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aPartList, passCache);

    // FBO (default framebuffer)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION("set_pipeline_state", CpuTime);
        // TODO handle pipeline state with an abstraction
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // We implemented alpha testing (cut-out), no blending.
        glDisable(GL_BLEND);
    }

    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        // TODO should be done once per viewport
        glViewport(0, 0,
                    mGlfwAppInterface->getFramebufferSize().width(),
                    mGlfwAppInterface->getFramebufferSize().height());

        // Reset texture bindings (to make sure that texturing does not work by "accident")
        // this could be extended to reset everything in the context.
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        {
            PROFILER_SCOPE_RECURRING_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, mUbos.mUboRepository, mDummyTextureRepository);
        }
    }
}

} // namespace ad::renderer