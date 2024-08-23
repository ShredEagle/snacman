#pragma once

#include "DrawQuad.h"

#include "graph/ShadowMapping.h"

#include <graphics/ApplicationGlfw.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Sampler.h>
#include <renderer/Texture.h>

#include <snac-renderer-V2/Cubemap.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Repositories.h>
#include <snac-renderer-V2/debug/DebugRenderer.h>


namespace ad::renderer {


class Camera;
struct Environment;
struct LightsData;
struct Loader;
struct Scene;
struct Storage;
struct ViewerPartList;
struct ViewerPassCache;


struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    RepositoryUbo mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;

    graphics::UniformBufferObject * mJointMatrixPaletteUbo;
    graphics::UniformBufferObject * mLightsUbo;
    graphics::UniformBufferObject * mLightViewProjectionUbo;
};


/// @brief Load data from aCamera into aUbo.
/// @note It is proving useful to have access to it to re-use passes outside of the main renderFrame()
void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera);


/// @brief The specific Render Graph for this viewer application.
struct TheGraph
{
    TheGraph(math::Size<2, int> aRenderSize,
             Storage & aStorage,
             const Loader & aLoader);

    void resize(math::Size<2, int> aNewSize);

    void allocateSizeDependentTextures(math::Size<2, int> aSize);

    void setupSizeDependentTextures();

    /// @param aFramebuffer Will be bound to DRAW for final image rendering.
    /// Its renderable size should match the current render size of this, as defined via resize().
    void renderFrame(const Scene & aScene,
                     const Camera & aCamera,
                     const LightsData & aLights_camera,
                     Storage & aStorage,
                     bool aShowTextures,
                     const graphics::FrameBuffer & aFramebuffer);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    // Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
    void passOpaqueDepth(const ViewerPartList & aPartList,
                         const RepositoryTexture & aTextureRepository,
                         Storage & mStorage) const;
    void passForward(const ViewerPartList & aPartList,
                     const RepositoryTexture & aTextureRepository,
                     Storage & aStorage,
                     bool aEnvironmentMappingconst);
    void passTransparencyAccumulation(const ViewerPartList & aPartList,
                                      const RepositoryTexture & aTextureRepository,
                                      Storage & mStorage);
    void passTransparencyResolve(const ViewerPartList & aPartList, 
                                 Storage & mStorage);
    void passDrawSkybox(const Environment & aEnvironment,
                        Storage & aStorage,
                        GLenum aCulledFace = GL_FRONT) const;

    void passSkyboxBase(const IntrospectProgram & aProgram, const Environment & aEnvironment, Storage & aStorage, GLenum aCulledFace) const;

    void loadDrawBuffers(const ViewerPassCache & aPassCache) const;

    void showTexture(const graphics::Texture & aTexture,
                     unsigned int aStackPosition,
                     DrawQuadParameters aDrawParams = {});

    void showDepthTexture(const graphics::Texture & aTexture,
                          float aNearZ, float aFarZ,
                          unsigned int aStackPosition = 0);
                        
    void drawUi();

// Data members:
    struct Controls
    {
        inline static const std::vector<StringKey> gForwardKeys{
            "forward",
            "forward_pbr",
            "forward_phong",
            "forward_debug",
        };

        std::vector<StringKey>::const_iterator mForwardPassKey = gForwardKeys.begin() + 1;

        inline static const std::array<GLenum, 3> gPolygonModes{
            GL_POINT,
            GL_LINE,
            GL_FILL,
        };

        std::array<GLenum, 3>::const_iterator mForwardPolygonMode = gPolygonModes.begin() + 2;
    };
    Controls mControls;

    HardcodedUbos mUbos;

    GenericStream mInstanceStream;

    graphics::BufferAny mIndirectBuffer;

    math::Size<2, int> mRenderSize;

    // Note Ad 2023/11/28: Might become a RenderTarger for optimal access if it is never sampled
    graphics::Texture mDepthMap{GL_TEXTURE_2D}; // The camera depth-map
    graphics::FrameBuffer mDepthFbo;

    //
    // Blended Order Independent Transparency
    // see: https://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html
    //
    graphics::Texture mTransparencyAccum{GL_TEXTURE_2D};
    graphics::Texture mTransparencyRevealage{GL_TEXTURE_2D};
    graphics::FrameBuffer mTransparencyFbo;
    QuadDrawer mTransparencyResolver;
    static const GLint gAccumTextureUnit{0};
    static const GLint gRevealageTextureUnit{1};

    // Shadow mapping
    ShadowMapping mShadowPass;
    Handle<graphics::Texture> mShadowMap;

    // Skybox rendering
    Skybox mSkybox;

    // Debug texture rendering (this should be encapsulated somewhere else)
    graphics::Sampler mShowTextureSampler;
};


} // namespace ad::renderer