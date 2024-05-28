#pragma once

#include "DrawQuad.h"

#include <graphics/ApplicationGlfw.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Texture.h>

#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Repositories.h>
#include <snac-renderer-V2/debug/DebugRenderer.h>


namespace ad::renderer {


class Camera;
struct Loader;
struct ViewerPartList;
struct ViewerPassCache;
struct Storage;


struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    RepositoryUbo mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;

    graphics::UniformBufferObject * mJointMatrixPaletteUbo;
};


/// @brief The specific Render Graph for this viewer application.
struct TheGraph
{
    TheGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
             Storage & aStorage,
             const Loader & aLoader);

    void renderFrame(const ViewerPartList & aPartList, 
                     const Camera & aCamera,
                     Storage & aStorage);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    // Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
    void passOpaqueDepth(const ViewerPartList & aPartList, Storage & mStorage);
    void passForward(const ViewerPartList & aPartList, Storage & mStorage);
    void passTransparencyAccumulation(const ViewerPartList & aPartList, Storage & mStorage);
    void passTransparencyResolve(const ViewerPartList & aPartList, Storage & mStorage);

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
            "forward_debug",
        };

        std::vector<StringKey>::const_iterator mForwardPassKey = gForwardKeys.begin();
    };
    Controls mControls;

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;

    HardcodedUbos mUbos;
    RepositoryTexture mDummyTextureRepository;

    GenericStream mInstanceStream;

    graphics::BufferAny mIndirectBuffer;

    math::Size<2, int> mRenderSize;

    // Note Ad 2023/11/28: Might become a RenderTarger for optimal access if it is never sampled
    graphics::Texture mDepthMap{GL_TEXTURE_2D};
    graphics::FrameBuffer mDepthFbo;

    // Transparency
    graphics::Texture mTransparencyAccum{GL_TEXTURE_2D};
    graphics::Texture mTransparencyRevealage{GL_TEXTURE_2D};
    graphics::FrameBuffer mTransparencyFbo;
    QuadDrawer mTransparencyResolver;
    static const GLint gAccumTextureUnit{0};
    static const GLint gRevealageTextureUnit{1};

    // Debug rendering
    DebugRenderer mDebugRenderer;

};


} // namespace ad::renderer