#include "ConsolidateGridMovement.h"

#include "math/VectorUtilities.h"

#include "../component/PathToOnGrid.h"
#include "../component/PlayerRoundData.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/AllowedMovement.h"

#include "../GameContext.h"
#include "../typedef.h"

#include <snacman/DebugDrawing.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {
constexpr float gBaseDogSpeed = 8.f;

ConsolidateGridMovement::ConsolidateGridMovement(GameContext & aGameContext) :
    mPlayer{aGameContext.mWorld}, mPathfinder{aGameContext.mWorld}
{}

void ConsolidateGridMovement::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);
    mPlayer.each([](const component::AllowedMovement & aAllowedMovement,
                    component::Controller & aController,
                    component::PlayerRoundData & aRoundData) {
        int oldMoveState = aRoundData.mMoveState;
        int inputMoveFlag = gPlayerMoveFlagNone;

        inputMoveFlag = aController.mInput.mCommand;

        inputMoveFlag &= aAllowedMovement.mAllowedMovement;

        bool canMove = aRoundData.mInvulFrameCounter <= 0.f;

        if (!canMove)
        {
            inputMoveFlag = gPlayerMoveFlagNone;
        }
        else if (inputMoveFlag == gPlayerMoveFlagNone)
        {
            inputMoveFlag = oldMoveState & aAllowedMovement.mAllowedMovement;
        }

        aRoundData.mMoveState = inputMoveFlag;
    });

    mPathfinder.each([aDelta](
                         const component::AllowedMovement & aAllowedMovement,
                         component::Geometry & aGeometry,
                         component::PathToOnGrid & aPathfinder,
                         component::GlobalPose & aPose) {
        if (aPathfinder.mTargetFound)
        {
            Vec2 direction = Pos2{(float) aPathfinder.mCurrentTarget.x(),
                                  (float) aPathfinder.mCurrentTarget.y()}
                             - aGeometry.mPosition.xy();

            if (direction.x() != 0.f || direction.y() != 0.f)
            {
                float distSquared = direction.getNormSquared();
                Vec2 displacement =
                    direction.normalize() * aDelta * gBaseDogSpeed;

                if (displacement.getNormSquared() < distSquared)
                {
                    aGeometry.mPosition +=
                        Vec3{displacement.x(), displacement.y(), 0.f};

                    math::Angle<float> dirAngle{
                        (float) getOrientedAngle(Vec2{0.f, 1.f}, -displacement)
                            .value()};
                    /* aGeometry.mOrientation = */
                    /*     Quat_f{UnitVec3{Vec3{0.f, 1.f, 0.f}}, dirAngle}; */
                    aGeometry.mOrientation =
                        Quat_f{UnitVec3{Vec3{1.f, 0.f, 0.f}}, Turn_f{0.25f}}
                        * Quat_f{UnitVec3{Vec3{0.f, 1.f, 0.f}}, dirAngle};
                    DBGDRAW_DEBUG(snac::gHitboxDrawer)
                        .addBasis({.mPosition = aPose.mPosition,
                                   .mOrientation = aPose.mOrientation});
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
