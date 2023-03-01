#include "IntegratePlayerMovement.h"

#include "snac-renderer/Cube.h"
#include "snacman/simulations/snacgame/SnacGame.h"

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
            int intPosX = static_cast<int>(aGeometry.mPosition.x() + gCellSize / 2);
            int intPosY = static_cast<int>(aGeometry.mPosition.y() + gCellSize / 2);
            if (aMoveState.mMoveState & gPlayerMoveFlagUp)
            {
                aGeometry.mPosition.y() +=
                    1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.x() = static_cast<float>(intPosX);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
            {
                aGeometry.mPosition.y() +=
                    -1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.x() = static_cast<float>(intPosX);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
            {
                aGeometry.mPosition.x() +=
                    -1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.y() = static_cast<float>(intPosY);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
            {
                aGeometry.mPosition.x() +=
                    1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.y() = static_cast<float>(intPosY);
            }
        });
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
