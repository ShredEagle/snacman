#include "DeterminePlayerAction.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/Logging.h"
#include <cmath>
#include <snacman/Profiling.h>

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"

namespace ad {
namespace snacgame {
namespace system {

constexpr float gTurningZoneHalfWidth = 0.1f;

void DeterminePlayerAction::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase nomutation;

    mLevel.each([&](component::LevelData & aLevelData,
                    component::LevelCreated &) {
        auto tiles = aLevelData.mTiles;
        int colCount = aLevelData.mSize.height();
        mPlayer.each([&](
                         component::Controller & aController,
                         component::Geometry & aPlayerGeometry,
                         component::PlayerMoveState & aPlayerMoveState) {
            int allowedMovementFlag = component::gAllowedMovementNone;
            int oldMoveState = aPlayerMoveState.mMoveState;
            int inputMoveFlag = gPlayerMoveFlagNone;

            int intPosX = static_cast<int>(aPlayerGeometry.mPosition.x() + 0.5f);
            int intPosY = static_cast<int>(aPlayerGeometry.mPosition.y() + 0.5f);
            float fracPosX = aPlayerGeometry.mPosition.x() - intPosX;
            float fracPosY = aPlayerGeometry.mPosition.y() - intPosY;

            auto pathUnderPlayerAllowedMove =
                tiles.at(intPosX
                         + intPosY * colCount);

            if (fracPosY < 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementUp
                    && fracPosX
                           > -gTurningZoneHalfWidth
                    && fracPosX
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementUp;
            }
            if (fracPosY > 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementDown
                    && fracPosX
                           > -gTurningZoneHalfWidth
                    && fracPosX
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementDown;
            }
            if (fracPosX > 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementLeft
                    && fracPosY
                           > -gTurningZoneHalfWidth
                    && fracPosY
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementLeft;
            }
            if (fracPosX < 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementRight
                    && fracPosY
                           > -gTurningZoneHalfWidth
                    && fracPosY
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementRight;
            }
            aPlayerMoveState.mAllowedMove = allowedMovementFlag;

            inputMoveFlag = aController.mCommandQuery;

            inputMoveFlag &= allowedMovementFlag;

            if (inputMoveFlag == gPlayerMoveFlagNone)
            {
                inputMoveFlag = oldMoveState & allowedMovementFlag;
            }

            aPlayerMoveState.mMoveState = inputMoveFlag;
        });
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
