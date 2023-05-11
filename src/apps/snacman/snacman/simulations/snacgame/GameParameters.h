#pragma once

#include <math/Angle.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {

constexpr int gGridSize = 29;
constexpr float gCellSize = 1.f;
constexpr float gPlayerTurningZoneHalfWidth = 0.4f;
constexpr float gOtherTurningZoneHalfWidth = 0.05f;
constexpr int gPointPerPill = 10;
constexpr int gMaxPlayerSlots = 4;


static const math::Quaternion<float> gWorldCoordTo3dCoordQuat = math::Quaternion<float>{
    math::UnitVec<3, float>::MakeFromUnitLength({-1.f, 0.f, 0.f}),
    math::Turn<float>{0.25f}};
static const math::LinearMatrix<3, 3, float> gWorldCoordTo3dCoord =
    gWorldCoordTo3dCoordQuat.toRotationMatrix();

} // namespace snacgame
} // namespace ad
