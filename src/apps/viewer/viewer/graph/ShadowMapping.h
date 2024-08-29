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
        GLfloat mSlopeScale{5.0f};
        GLfloat mUnitScale{5000.f};

        bool mTightenLightFrustumToScene{true};
    };

    Controls mControls;

    graphics::FrameBuffer mFbo;

    static constexpr math::Size<2, GLsizei> gTextureSize{2048, 2048};
};


template <class T_visitor>
void r(T_visitor & aV, ShadowMapping::Controls & aControls)
{
    give(aV, aControls.mSlopeScale, "slope scale");
    give(aV, aControls.mUnitScale,  "unit scale");
    give(aV, aControls.mTightenLightFrustumToScene,  "frustum to scene");
}


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const TheGraph & aGraph,  // TODO Ad 2024/08/23: This should be decoupled, but requires a redesign
                                  const ViewerPartList & aPartList,
                                  math::Box<GLfloat> aSceneAabb,
                                  math::Matrix<4, 4, GLfloat> aViewProjectionInverse,
                                  Handle<const graphics::Texture> aShadowMap,
                                  std::span<const DirectionalLight> aDirectionalLights,
                                  bool aDebugDrawFrusta);


} // namespace ad::renderer