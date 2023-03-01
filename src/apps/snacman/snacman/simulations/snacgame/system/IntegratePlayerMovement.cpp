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
                aGeometry.mPosition.y() +=
                    1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.x() = static_cast<int>(aGeometry.mPosition.x() + 0.5f);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagDown)
            {
                aGeometry.mPosition.y() +=
                    -1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.x() = static_cast<int>(aGeometry.mPosition.x() + 0.5f);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagLeft)
            {
                aGeometry.mPosition.x() +=
                    -1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.y() = static_cast<int>(aGeometry.mPosition.y() + 0.5f);
            }
            else if (aMoveState.mMoveState & gPlayerMoveFlagRight)
            {
                aGeometry.mPosition.x() +=
                    1.f * gBasePlayerSpeed * aDelta;
                aGeometry.mPosition.y() = static_cast<int>(aGeometry.mPosition.y() + 0.5f);
            }
        });
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
