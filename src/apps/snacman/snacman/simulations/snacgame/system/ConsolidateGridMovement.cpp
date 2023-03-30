#include "ConsolidateGridMovement.h"

#include "snacman/Logging.h"
#include "snacman/Profiling.h"

#include "../typedef.h"

namespace ad {
namespace snacgame {
namespace system {
constexpr float gBaseDogSpeed = 8.f;

void ConsolidateGridMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);
    mPlayer.each([](const component::AllowedMovement & aAllowedMovement,
                    component::Controller & aController,
                    component::PlayerMoveState & aMoveState) {
        int oldMoveState = aMoveState.mMoveState;
        int inputMoveFlag = gPlayerMoveFlagNone;

        inputMoveFlag = aController.mCommandQuery;

        inputMoveFlag &= aAllowedMovement.mAllowedMovement;

        if (inputMoveFlag == gPlayerMoveFlagNone)
        {
            inputMoveFlag = oldMoveState & aAllowedMovement.mAllowedMovement;
        }

        aMoveState.mMoveState = inputMoveFlag;
    });

    mPathfinder.each(
        [aDelta](const component::AllowedMovement & aAllowedMovement,
                 component::Geometry & aGeometry,
                 component::PathToOnGrid & aPathfinder) {
            if (aPathfinder.mTargetFound)
            {
                Vec2 direction = Pos2{(float) aPathfinder.mCurrentTarget.x(),
                                      (float) aPathfinder.mCurrentTarget.y()}
                                 - aGeometry.mPosition.xy();

                if (direction.x() != 0.f || direction.y() != 0.f)
                {
                    direction = direction.normalize();

                    aGeometry.mPosition +=
                        Vec3{direction.x(), direction.y(), 0.f} * aDelta * gBaseDogSpeed;
                }
            }
        });
}
} // namespace system
} // namespace snacgame
} // namespace ad
