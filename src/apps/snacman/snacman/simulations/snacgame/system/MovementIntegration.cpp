#include "MovementIntegration.h"

#include "../GameContext.h"

#include "../component/Geometry.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PoseScreenSpace.h"
#include "../component/Speed.h"

#include <snacman/Profiling.h>

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

MovementIntegration::MovementIntegration(GameContext & aGameContext) :
    mMovables{aGameContext.mWorld}, mScreenMovables{aGameContext.mWorld}
{}

void MovementIntegration::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase integrate;
    mMovables.each(
        [aDelta]
        (const component::Speed & aMovement, component::Geometry & aPose) 
        {
            math::Quaternion rotation{aMovement.mRotation.mAxis, aDelta * aMovement.mRotation.mAngle};
            // Quaternion multiplication perform rotation from right to left
            aPose.mOrientation = rotation * aPose.mOrientation;
            aPose.mPosition += aDelta * aMovement.mSpeed;
        }
    );
    mScreenMovables.each(
        [aDelta]
        (const component::MovementScreenSpace & aMovement, component::PoseScreenSpace & aPose) 
        {
            aPose.mRotationCCW += aDelta * aMovement.mAngularSpeed;
        }
    );
}

} // namespace system
} // namespace snacgame
} // namespace ad
