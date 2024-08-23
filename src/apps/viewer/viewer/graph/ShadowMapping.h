#pragma once


#include "../Lights.h"

#include <renderer/FrameBuffer.h>

#include <snac-renderer-V2/Model.h>


namespace ad::renderer {


// Forward declaration of the problematic coupled class
struct TheGraph;
struct ViewerPartList;

// TODO Ad 2024/08/23: Turn that into a proper Pass abstraction
// but this abstraction has to be designed first.

struct ShadowMapping
{
    struct Controls
    {
        GLfloat mSlopeScale{0.01f};
        GLfloat mUnitScale{2.f};
    };

    Controls mControls;

    graphics::FrameBuffer mFbo;

    static constexpr math::Size<2, GLsizei> gTextureSize{2048, 2048};
};


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const TheGraph & aGraph,  // TODO Ad 2024/08/23: This should be decoupled, but requires a redesign
                                  const ViewerPartList & aPartList, // TODO remove by better splitting
                                  Handle<const graphics::Texture> aShadowMap,
                                  std::span<const DirectionalLight> aDirectionalLights,
                                  bool aDebugDrawFrusta);


} // namespace ad::renderer