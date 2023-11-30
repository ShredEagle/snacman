#pragma once

#include "DrawQuad.h"
#include "Model.h"
#include "Repositories.h"

#include <graphics/ApplicationGlfw.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Texture.h>


namespace ad::renderer {


class Camera;
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
};


struct TheGraph
{
    TheGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
             Storage & aStorage);

    void renderFrame(const PartList & aPartList, const Camera & aCamera, Storage & aStorage);

    // Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
    void passDepth(const PartList & aPartList, Storage & mStorage);
    void passForward(const PartList & aPartList, Storage & mStorage);

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

    math::Size<2, int> mDepthMapSize;
    // Note Ad 2023/11/28: Might become a RenderTarger for optimal access if it is never sampled
    graphics::Texture mDepthMap{GL_TEXTURE_2D};
    graphics::FrameBuffer mDepthFbo;
};


} // namespace ad::renderer