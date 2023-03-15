#include "IntegratePlayerMovement.h"
#include "../GameParameters.h"

#include "snac-renderer/Cube.h"

#include "../component/LevelData.h"

#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

constexpr float gBasePlayerSpeed = 5.f;

void IntegratePlayerMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase nomutation;

    mPlayer.each([aDelta](
                     component::Geometry & aGeometry,
                     const component::PlayerMoveState & aMoveState) {
        int intPosX = static_cast<int>(aGeometry.mPosition.x() + gCellSize / 2);
        int intPosY = static_cast<int>(aGeometry.mPosition.y() + gCellSize / 2);
        if (aMoveState.mMoveState & gPlayerMoveFlagUp)
        {
            aGeometry.mPosition.y() +=
                1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPosX);
            aGeometry.mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                        {0.f, 1.f, 0.f}},
                        math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
        {
            aGeometry.mPosition.y() +=
                -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPosX);
            aGeometry.mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                        {0.f, 1.f, 0.f}},
                        math::Turn<float>{0.75f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
        {
            aGeometry.mPosition.x() +=
                -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPosY);
            aGeometry.mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                        {0.f, 1.f, 0.f}},
                        math::Turn<float>{0.5f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
        {
            aGeometry.mPosition.x() +=
                1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPosY);
            aGeometry.mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                        {0.f, 1.f, 0.f}},
                        math::Turn<float>{0.f}};
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
