#include "IntegratePlayerMovement.h"
#include "snacman/simulations/snacgame/component/RigAnimation.h"

#include "../LevelHelper.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include "../component/LevelData.h"
#include "../component/VisualModel.h"

#include <snacman/EntityUtilities.h>
#include <snacman/Profiling.h>

#include <snac-renderer/Cube.h>

namespace ad {
namespace snacgame {
namespace system {

constexpr float gBasePlayerSpeed = 5.f;

void IntegratePlayerMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase updateOrientation;

    mPlayer.each([aDelta, &updateOrientation](component::Geometry & aGeometry,
                          const component::PlayerMoveState & aMoveState,
                          component::PlayerModel & aModel) {
        Pos2_i intPos = getLevelPosition_i(aGeometry.mPosition.xy());
        Quat_f & orientation = aModel.mModel.get(updateOrientation)->get<component::Geometry>().mOrientation;
        if (aMoveState.mMoveState & gPlayerMoveFlagUp)
        {
            aGeometry.mPosition.y() += 1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPos.x());
            orientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.25f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
        {
            aGeometry.mPosition.y() += -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.x() = static_cast<float>(intPos.x());
            orientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.75f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
        {
            aGeometry.mPosition.x() += -1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPos.y());
            orientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.5f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }
        else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
        {
            aGeometry.mPosition.x() += 1.f * gBasePlayerSpeed * aDelta;
            aGeometry.mPosition.y() = static_cast<float>(intPos.y());
            orientation =
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                    math::Turn<float>{0.f}} *
                math::Quaternion<float>{
                    math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                    math::Turn<float>{0.25f}};
        }

        if (aModel.mModel.get()->has<component::VisualModel>())
        {
            snac::getComponent<component::VisualModel>(aModel.mModel)
                .mDisableInterpolation = true;
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
