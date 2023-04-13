#include "ConsolidateGridMovement.h"

#include "snacman/Logging.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/component/PlayerLifeCycle.h"

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
                    component::PlayerMoveState & aMoveState,
                    const component::PlayerLifeCycle & aLifeCycle ) {
        int oldMoveState = aMoveState.mMoveState;
        int inputMoveFlag = gPlayerMoveFlagNone;

        inputMoveFlag = aController.mCommandQuery;

        inputMoveFlag &= aAllowedMovement.mAllowedMovement;

        bool canMove = aLifeCycle.mHitStun <= 0.f;

        if (!canMove)
        {
            inputMoveFlag = gPlayerMoveFlagNone;
        }
        else if (inputMoveFlag == gPlayerMoveFlagNone)
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
                    float distSquared = direction.getNormSquared();
                    Vec2 displacement = direction.normalize() * aDelta * gBaseDogSpeed;

                    if (displacement.getNormSquared() < distSquared)
                    {
                        aGeometry.mPosition +=
                            Vec3{displacement.x(), displacement.y(), 0.f};
                    }
                    else
                    {
                        aGeometry.mPosition = {
                            aPathfinder.mCurrentTarget.x(),
                            aPathfinder.mCurrentTarget.y(),
                            aGeometry.mPosition.z(),
                        };
                    }
                    
                }
            }
        });
}
} // namespace system
} // namespace snacgame
} // namespace ad
