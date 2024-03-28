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
struct PartList;
struct PassCache;
struct Storage;


void draw(const PassCache & aPassCache,
          const RepositoryUbo & aUboRepository,
          const RepositoryTexture & aTextureRepository);


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

    void renderFrame(const PartList & aPartList, 
                     const Camera & aCamera,
                     Storage & aStorage);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    // Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
    void passOpaqueDepth(const PartList & aPartList, Storage & mStorage);
    void passForward(const PartList & aPartList, Storage & mStorage);
    void passTransparencyAccumulation(const PartList & aPartList, Storage & mStorage);
    void passTransparencyResolve(const PartList & aPartList, Storage & mStorage);

    void loadDrawBuffers(const PartList & aPartList, const PassCache & aPassCache);

    void showTexture(const graphics::Texture & aTexture,
                     unsigned int aStackPosition,
                     DrawQuadParameters aDrawParams = {});

    void showDepthTexture(const graphics::Texture & aTexture,
                          float aNearZ, float aFarZ,
                          unsigned int aStackPosition = 0);
// Data members:
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