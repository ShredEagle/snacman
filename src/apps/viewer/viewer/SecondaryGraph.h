#pragma once

#include "DrawQuad.h"
#include "GraphCommon.h"

#include <graphics/ApplicationGlfw.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Sampler.h>
#include <renderer/Texture.h>

#include <snac-renderer-V2/Cubemap.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Repositories.h>
#include <snac-renderer-V2/debug/DebugRenderer.h>
#include <snac-renderer-V2/graph/ShadowMapping.h>


namespace ad::renderer {


class Camera;
struct Environment;
struct GpuViewProjectionBlock;
struct LightsDataCommon;
struct Loader;
struct Scene;
struct Storage;
struct ViewerPartList;
struct ViewerPassCache;


/// @brief The specific Render Graph for this viewer application.
struct SecondaryGraph
{
    SecondaryGraph(math::Size<2, int> aRenderSize,
                   const Loader & aLoader);

    void resize(math::Size<2, int> aNewSize);

    void allocateSizeDependentTextures(math::Size<2, int> aSize);

    void setupSizeDependentTextures();

    /// @param aFramebuffer Will be bound to DRAW for final image rendering.
    /// Its renderable size should match the current render size of this, as defined via resize().
    void renderFrame(const Scene & aScene,
                     const ViewerPartList & aPartList,
                     const Camera & aCamera,
                     const LightsDataCommon & aLights_camera,
                     const GraphShared & aGraphShared,
                     Storage & aStorage,
                     bool aShowTextures,
                     const graphics::FrameBuffer & aFramebuffer);

    void renderDebugDrawlist(snac::DebugDrawer::DrawList aDrawList, Storage & aStorage);

    void drawUi();

// Data members:
    GraphControls mControls;

    math::Size<2, int> mRenderSize;

    // Note Ad 2023/11/28: Might become a RenderTarger for optimal access if it is never sampled
    graphics::Texture mDepthMap{GL_TEXTURE_2D}; // The camera depth-map
    graphics::FrameBuffer mDepthFbo;
};


} // namespace ad::renderer