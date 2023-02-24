#include "IntegratePlayerMovement.h"

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

    mLevel.each([&](component::LevelData & aLevelData,
                    component::LevelCreated &) {
        mPlayer.each([aDelta](
                         component::Geometry & aGeometry,
                         const component::PlayerMoveState & aMoveState) {
            if (aMoveState.mMoveState & gPlayerMoveFlagUp)
            {
                aGeometry.mPosition +=
                    math::Vec<2, float>{0.f, -1.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
            {
                aGeometry.mPosition +=
                    math::Vec<2, float>{0.f, 1.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
            {
                aGeometry.mPosition +=
                    math::Vec<2, float>{1.f, 0.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
            {
                aGeometry.mPosition +=
                    math::Vec<2, float>{-1.f, 0.f} * gBasePlayerSpeed * aDelta;
            }
        });
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
