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
    /// @brief Allocate texture and setup their parameters, respecting per-light budget.
    /// (CSM will use smaller layers than single).
    void prepareShadowMap();

    /// @brief Ensures the current setup matches the configured controls
    /// and fix it if necessary
    void reviewControls();

    // Note: #thread-safety this will not be thread safe if controls are presented in imgui on the main thread
    // and values read by a render thread.
    struct Controls
    {
        GLfloat mSlopeScale{2.0f};
        GLfloat mUnitScale{5000.f};

        // Toggle cascaded shadow map.
        bool mUseCascades{true};

        // Toggle hardware percentage closer filtering
        bool mUseHardwarePcf{true};

        bool mDebugDrawFrusta{true};
        bool mDebugDrawShadowPlanes{false};

        // Toggle wether we move the light frustum in texel-sized increments
        // to limit shimmering
        // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN#moving-the-light-in-texel-sized-increments
        bool mMoveFrustumTexelIncrement{true};

        // Note: it is not always recommended, because it can change the light frustum size
        // frame to frame, which might degrade light texel grid alignment techniques.
        // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN#calculating-a-tight-projection
        bool mTightenLightFrustumXYToScene{true};

        // When enabled, the world-AABB of the scene is clipped to the the 4 X and Y bounds of the shadow frustum
        // to determine tighter Near and Far for the shadow frustum.
        // (Otherwise, Near and Far are determined by the whole unclipped scene bounding box)
        bool mTightenFrustumDepthToClippedScene{true};
        bool mDebugDrawClippedTriangles{false};

        GLfloat mCsmNearPlaneLimit{-1.f};
        int mDebugDrawWhichCascade{0};
        bool mTintViewByCascadeIdx{false};
    };

    Controls mControls;

    graphics::FrameBuffer mFbo;

    Handle<graphics::Texture> mShadowMap;
    math::Size<2, GLsizei> mMapSize;

    // TODO Ad 2024/10/17: #ui find a better pattern to implement the controllers of values
    // that toggle an effect only when their value is changed.
    bool mIsSetupForCsm = mControls.mUseCascades;
    bool mIsSetupForPcf = mControls.mUseHardwarePcf;

    // This is not necessarily the size of and individual texture layer, but the budget for a single light:
    // For CSM, this budget should be divided by the number of cascades.
    static constexpr math::Size<2, GLsizei> gTexelPerLight{2048, 2048};
};


template <class T_visitor>
void r(T_visitor & aV, ShadowMapping::Controls & aControls)
{
    give(aV, aControls.mSlopeScale, "slope scale");
    give(aV, aControls.mUnitScale, "unit scale");
    give(aV, aControls.mUseCascades, "Cascaded Shadow Maps");
    give(aV, aControls.mUseHardwarePcf, "Percentage Closer Filtering hw");
    give(aV, aControls.mDebugDrawFrusta, "debug: Show camera & light frusta");
    give(aV, aControls.mDebugDrawShadowPlanes, "debug: Show shadow planes (light frusta X-Y bounds)");
    give(aV, aControls.mMoveFrustumTexelIncrement,  "Move light frustum in texel increments");
    give(aV, aControls.mTightenLightFrustumXYToScene,  "frustum XY to scene");
    give(aV, aControls.mTightenFrustumDepthToClippedScene,  "frustum Z to clipped scene");
    give(aV, aControls.mDebugDrawClippedTriangles,  "debug: Draw clipped triangles");
    give(aV, aControls.mCsmNearPlaneLimit,  "The near plane will be clamped for cascade partitionning");
    // We allow the cascade index to go 1 above max index, to mean "all cascades"
    give(aV, Clamped<int>{aControls.mDebugDrawWhichCascade, -1, gCascadesPerShadow/* - 1*/},
         "debug: Draw shadow cascade frustum");
    give(aV, aControls.mTintViewByCascadeIdx,  "debug: Tint rendering by cascade index");
}


/// @brief Prepare all buffers and textures (shadow maps) required for shadow mapping.
/// @param aPartList All the potential shadow casters.
/// @param aSceneAabb_world The bounding box of the whole scene, in world coordinates.
/// @param aCamera The camera for which a shadow mapped rendering will occur.
/// @param aLights A collection of lights, some of which might project a shadow (up to gMaxShadowLights)
/// @return A buffer associating each light to a base shadow map index, or InvalidIndex if the light does not project.
/// 
/// This function is loading the shadow cascade and light-view-projections UBOs, render the shadow map textures,
/// and return part of the LightsData UBO to be loaded by the caller.
LightsDataInternal fillShadowMap(const ShadowMapping & aPass, 
                                 const RepositoryTexture & aTextureRepository,
                                 Storage & aStorage,
                                 const GraphShared & aGraphShared,
                                 const ViewerPartList & aPartList,
                                 math::Box<GLfloat> aSceneAabb_world,
                                 const Camera & aCamera,
                                 const LightsDataUi & aLights);


} // namespace ad::renderer