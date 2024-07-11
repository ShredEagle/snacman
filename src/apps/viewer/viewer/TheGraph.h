#pragma once

#include "DrawQuad.h"

#include <graphics/ApplicationGlfw.h>

#include <renderer/FrameBuffer.h>
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
};


/// @brief Load data from aCamera into aUbo.
/// @note It is proving useful to have access to it to re-use passes outside of the main renderFrame()
void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera);


/// @brief The specific Render Graph for this viewer application.
struct TheGraph
{
    TheGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
             Storage & aStorage,
             const Loader & aLoader);

    void onFramebufferResize(math::Size<2, int> aNewSize);

    void allocateTextures(math::Size<2, int> aSize);

    void setupTextures();

    void renderFrame(const Scene & aScene,
                     const Camera & aCamera,
                     const LightsData & aLights_camera,
                     Storage & aStorage);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    // Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
    void passOpaqueDepth(const ViewerPartList & aPartList, Storage & mStorage);
    void passForward(const ViewerPartList & aPartList,
                     Storage & aStorage,
                     bool aEnvironmentMappingconst);
    void passTransparencyAccumulation(const ViewerPartList & aPartList, Storage & mStorage);
    void passTransparencyResolve(const ViewerPartList & aPartList, Storage & mStorage);
    void passDrawSkybox(const Environment & aEnvironment, Storage & aStorage) const;
    void passFilterEnvironment(const Environment & aEnvironment, float aRoughness, Storage & aStorage) const;

    void passSkyboxBase(const IntrospectProgram & aProgram, const Environment & aEnvironment, Storage & aStorage) const;

    void loadDrawBuffers(const ViewerPartList & aPartList, const ViewerPassCache & aPassCache);

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

        std::vector<StringKey>::const_iterator mForwardPassKey = gForwardKeys.begin();

        inline static const std::array<GLenum, 3> gPolygonModes{
            GL_POINT,
            GL_LINE,
            GL_FILL,
        };

        std::array<GLenum, 3>::const_iterator mForwardPolygonMode = gPolygonModes.begin() + 2;
    };
    Controls mControls;

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;

    HardcodedUbos mUbos;
    RepositoryTexture mTextureRepository;

    GenericStream mInstanceStream;

    graphics::BufferAny mIndirectBuffer;

    std::shared_ptr<graphics::AppInterface::SizeListener> mFramebufferSizeListener;

    math::Size<2, int> mRenderSize;

    // Note Ad 2023/11/28: Might become a RenderTarger for optimal access if it is never sampled
    graphics::Texture mDepthMap{GL_TEXTURE_2D};
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

    // Skybox rendering
    Skybox mSkybox;

    // Debug rendering
    DebugRenderer mDebugRenderer;

};


} // namespace ad::renderer