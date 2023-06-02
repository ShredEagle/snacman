#include "IntegratePlayerMovement.h"

#include "../LevelHelper.h"
#include "../GameParameters.h"
#include "../typedef.h"
#include "../GameContext.h"

#include "../component/PlayerRoundData.h"
#include "../component/Geometry.h"
#include "../component/RigAnimation.h"
#include "../component/LevelData.h"
#include "../component/VisualModel.h"

#include <snacman/EntityUtilities.h>
#include <snacman/Profiling.h>

#include <snac-renderer/Cube.h>

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

constexpr float gBasePlayerSpeed = 5.f;

IntegratePlayerMovement::IntegratePlayerMovement(GameContext & aGameContext) :
    mPlayer{aGameContext.mWorld}
{}

void IntegratePlayerMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mPlayer.each([aDelta](component::Geometry & aGeometry,
                          component::PlayerRoundData & aRoundData) {
        Pos2_i intPos = getLevelPosition_i(aGeometry.mPosition.xy());
        Quat_f & orientation = aRoundData.mModel.get()->get<component::Geometry>().mOrientation;
        if (aRoundData.mMoveState & gPlayerMoveFlagUp)
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
        else if (aRoundData.mMoveState & gPlayerMoveFlagDown)
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
        else if (aRoundData.mMoveState & gPlayerMoveFlagLeft)
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
        else if (aRoundData.mMoveState & gPlayerMoveFlagRight)
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

        if (aRoundData.mModel.get()->has<component::VisualModel>())
        {
            snac::getComponent<component::VisualModel>(aRoundData.mModel)
                .mDisableInterpolation = true;
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
