#pragma once

#include "DrawQuad.h"
#include "GraphCommon.h"

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


/// @brief The specific Render Graph for this viewer application.
struct TheGraph
{
    TheGraph(math::Size<2, int> aRenderSize,
             Storage & aStorage,
             const Loader & aLoader);

    void resize(math::Size<2, int> aNewSize);

    void allocateSizeDependentTextures(math::Size<2, int> aSize);

    void setupSizeDependentTextures();

    // TODO Ad 2024/10/04: Sort-out this function signature. In particular, scene and partlist are a repetition
    // (the partlist is likely common to all graphs rendering the same scene, so is computed above)
    /// @param aFramebuffer Will be bound to DRAW for final image rendering.
    /// Its renderable size should match the current render size of this, as defined via resize().
    void renderFrame(const Scene & aScene,
                     const ViewerPartList & aPartList,
                     const Camera & aCamera,
                     const LightsData & aLights_camera,
                     const GraphShared & aGraphShared,
                     Storage & aStorage,
                     bool aShowTextures,
                     const graphics::FrameBuffer & aFramebuffer);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    void passTransparencyAccumulation(const ViewerPartList & aPartList,
                                      const RepositoryTexture & aTextureRepository,
                                      Storage & mStorage);
    void passTransparencyResolve(const ViewerPartList & aPartList, 
                                 Storage & mStorage);

    void showTexture(const graphics::Texture & aTexture,
                     unsigned int aStackPosition,
                     DrawQuadParameters aDrawParams = {});

    void showDepthTexture(const graphics::Texture & aTexture,
                          float aNearZ, float aFarZ,
                          unsigned int aStackPosition = 0);
                        
    void drawUi();

// Data members:
    GraphControls mControls;

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

    // Debug texture rendering (this should be encapsulated somewhere else)
    graphics::Sampler mShowTextureSampler;
};


} // namespace ad::renderer