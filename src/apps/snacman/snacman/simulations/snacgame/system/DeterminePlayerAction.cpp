#include "DeterminePlayerAction.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/Logging.h"
#include <snacman/Profiling.h>

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"

namespace ad {
namespace snacgame {
namespace system {

constexpr float gTurningZoneHalfWidth = 0.1f;

void DeterminePlayerAction::update()
{
    TIME_RECURRING_FULLFUNC;

    ent::Phase nomutation;

    mLevel.each([&](component::LevelData & aLevelData,
                    component::LevelCreated &) {
        auto tiles = aLevelData.mTiles;
        int colCount = aLevelData.mSize.height();
        mPlayer.each([colCount, &tiles](
                         component::Controller & aController,
                         component::Geometry & aPlayerGeometry,
                         component::PlayerMoveState & aPlayerMoveState) {
            int allowedMovementFlag = component::gAllowedMovementNone;
            int oldMoveState = aPlayerMoveState.mMoveState;
            int inputMoveFlag = gPlayerMoveFlagNone;

            auto pathUnderPlayerAllowedMove =
                tiles.at(aPlayerGeometry.mGridPosition.x()
                         + aPlayerGeometry.mGridPosition.y() * colCount);

            if (aPlayerGeometry.mSubGridPosition.y() > 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementUp
                    && aPlayerGeometry.mSubGridPosition.x()
                           > -gTurningZoneHalfWidth
                    && aPlayerGeometry.mSubGridPosition.x()
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementUp;
            }
            if (aPlayerGeometry.mSubGridPosition.y() < 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementDown
                    && aPlayerGeometry.mSubGridPosition.x()
                           > -gTurningZoneHalfWidth
                    && aPlayerGeometry.mSubGridPosition.x()
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementDown;
            }
            if (aPlayerGeometry.mSubGridPosition.x() < 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementLeft
                    && aPlayerGeometry.mSubGridPosition.y()
                           > -gTurningZoneHalfWidth
                    && aPlayerGeometry.mSubGridPosition.y()
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementLeft;
            }
            if (aPlayerGeometry.mSubGridPosition.x() > 0.f
                || (pathUnderPlayerAllowedMove.mAllowedMove
                        & component::gAllowedMovementRight
                    && aPlayerGeometry.mSubGridPosition.y()
                           > -gTurningZoneHalfWidth
                    && aPlayerGeometry.mSubGridPosition.y()
                           < gTurningZoneHalfWidth))
            {
                allowedMovementFlag |= component::gAllowedMovementRight;
            }

            if ((aPlayerGeometry.mSubGridPosition.y() < 0.f
                 && !(allowedMovementFlag & component::gAllowedMovementUp))
                || (aPlayerGeometry.mSubGridPosition.y() > 0.f
                    && !(allowedMovementFlag
                         & component::gAllowedMovementDown)))
            {
                aPlayerGeometry.mSubGridPosition.y() = 0.f;
            }
            if ((aPlayerGeometry.mSubGridPosition.x() > 0.f
                 && !(allowedMovementFlag & component::gAllowedMovementLeft))
                || (aPlayerGeometry.mSubGridPosition.x() < 0.f
                    && !(allowedMovementFlag
                         & component::gAllowedMovementRight)))
            {
                aPlayerGeometry.mSubGridPosition.x() = 0.f;
            }

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
