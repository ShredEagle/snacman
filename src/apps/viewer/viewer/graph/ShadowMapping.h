#pragma once


#include "Clipping.h"

#include "../Lights.h"

#include <renderer/FrameBuffer.h>

#include <snac-renderer-V2/Model.h>


namespace ad::renderer {


// Forward declaration of the problematic coupled class
class Camera;
struct GraphShared;
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

        // Note: it is not always recommended, because it can change the light frustum size
        // frame to frame, which might degrade light texel grid alignment techniques.
        // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN#calculating-a-tight-projection
        bool mTightenLightFrustumXYToScene{true};

        // When enabled, the world-AABB of the scene is clipped to the the 4 X and Y bounds of the shadow frustum
        // to determine tighter Near and Far for the shadow frustum.
        // (Otherwise, Near and Far are determined by the whole unclipped scene bounding box)
        bool mTightenFrustumDepthToClippedScene{true};
        bool mDebugDrawClippedTriangles{false};
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
    give(aV, aControls.mTightenLightFrustumXYToScene,  "frustum XY to scene");
    give(aV, aControls.mTightenFrustumDepthToClippedScene,  "frustum Z to clipped scene");
    give(aV, aControls.mDebugDrawClippedTriangles,  "debugdraw clipped triangles");
}


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const GraphShared & aGraphShared,
                                  const ViewerPartList & aPartList,
                                  math::Box<GLfloat> aSceneAabb,
                                  const Camera & aCamera,
                                  Handle<const graphics::Texture> aShadowMap,
                                  std::span<const DirectionalLight> aDirectionalLights,
                                  bool aDebugDrawFrusta);


} // namespace ad::renderer