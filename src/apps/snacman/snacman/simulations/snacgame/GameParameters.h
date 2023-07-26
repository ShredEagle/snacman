#pragma once

#include "math/Spherical.h"
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {

constexpr const char * gMenuSceneName = "Menu";
constexpr const char * gPauseSceneName = "Pause";
constexpr const char * gGameSceneName = "Game";
constexpr const char * gJoinGameSceneName = "JoinGame";

constexpr math::Spherical<float> gInitialCameraSpherical{
    20.f, math::Turn<float>{0.075f},
    math::Turn<float>{0.f}};

constexpr std::array<math::hdr::Rgba_f, 5> gSlotColors{
    math::hdr::Rgba_f{1.f, 1.f, 1.f, 1.f},
    math::hdr::Rgba_f{0.949f, 0.251f, 0.506f, 1.f},
    math::hdr::Rgba_f{0.961f, 0.718f, 0.f, 1.f},
    math::hdr::Rgba_f{0.f, 0.631f, 0.894f, 1.f},
    math::hdr::Rgba_f{0.725f, 1.f, 0.718f, 1.f}
};
constexpr int gMaxPlayerSlots = 4;

constexpr float gPlayerTurningZoneHalfWidth = 0.4f;
constexpr float gOtherTurningZoneHalfWidth = 0.05f;

constexpr int gGridSize = 29;
constexpr float gCellSize = 1.f;

constexpr int gPointPerPill = 10;

constexpr float gPillHeight = 6 * gCellSize * 0.1f;
constexpr float gPlayerHeight = 0 * gCellSize * 0.1f;
constexpr float gLevelHeight = 0 * gCellSize * 0.1f;

const math::Quaternion<float> gBaseCrownOrientation =
    math::Quaternion<float>{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                            math::Turn<float>{0.25f}}
    * math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                              math::Turn<float>{0.25f}};
constexpr math::Size<3, float> gBaseCrownInstanceScaling{0.2f, 0.2f, 0.2f};
constexpr math::Position<3, float> gBaseCrownPosition{0.f, 0.f, 1.8f};

const math::Quaternion<float> gBasePlayerModelOrientation =
    math::Quaternion<float>{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                            math::Turn<float>{0.25f}}
    * math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                              math::Turn<float>{0.25f}};
constexpr math::Size<3, float> gBasePlayerModelInstanceScaling{0.2f, 0.2f, 0.2f};

const math::Quaternion<float> gWorldCoordTo3dCoordQuat =
    math::Quaternion<float>{
        math::UnitVec<3, float>::MakeFromUnitLength({-1.f, 0.f, 0.f}),
        math::Turn<float>{0.25f}};
const math::LinearMatrix<3, 3, float> gWorldCoordTo3dCoord =
    gWorldCoordTo3dCoordQuat.toRotationMatrix();

const math::hdr::Rgba_f gColorItemUnselected = math::hdr::gBlack<float>;
const math::hdr::Rgba_f gColorItemSelected = math::hdr::gCyan<float>;

} // namespace snacgame
} // namespace ad
