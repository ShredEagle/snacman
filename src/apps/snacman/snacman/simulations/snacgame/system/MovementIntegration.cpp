#include "MovementIntegration.h"

#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {


void MovementIntegration::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC;

    ent::Phase integrate;
    mMovables.each(
        [aDelta]
        (const component::Speed & aMovement, component::Geometry & aPose) 
        {
            math::Quaternion rotation{aMovement.mRotation.mAxis, aDelta * aMovement.mRotation.mAngle};
            // Quaternion multiplication perform rotation from right to left
            aPose.mOrientation = rotation * aPose.mOrientation;
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