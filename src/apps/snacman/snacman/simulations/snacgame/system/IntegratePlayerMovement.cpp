#include "IntegratePlayerMovement.h"

#include "../LevelHelper.h"
#include "../GameParameters.h"

#include "../component/LevelData.h"

#include <snacman/Profiling.h>

#include <snac-renderer/Cube.h>

namespace ad {
namespace snacgame {
namespace system {

constexpr float gBasePlayerSpeed = 5.f;

void IntegratePlayerMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase nomutation;

    mPlayer.each([aDelta](component::Geometry & aGeometry,
                          const component::PlayerMoveState & aMoveState) {
        Pos2_i intPos = getLevelPosition_i(aGeometry.mPosition.xy());
        if (aMoveState.mMoveState & gPlayerMoveFlagUp)
        {
            aGeometry.mPosition.y() += 1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPos.x());
            aGeometry.mOrientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
        {
            aGeometry.mPosition.y() += -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPos.x());
            aGeometry.mOrientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.5f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
        {
            aGeometry.mPosition.x() += -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPos.y());
            aGeometry.mOrientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.25f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
        {
            aGeometry.mPosition.x() += 1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPos.y());
            aGeometry.mOrientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.75f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
