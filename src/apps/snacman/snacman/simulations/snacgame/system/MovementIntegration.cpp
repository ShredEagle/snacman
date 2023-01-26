#include "MovementIntegration.h"

namespace ad {
namespace snacgame {
namespace system {


void MovementIntegration::update(float aDelta)
{
    ent::Phase integrate;
    mMovables.each(
        [aDelta]
        (const component::Speed & aMovement, component::Geometry & aPose) 
        {
            // TODO understand how to represent rotational velocity here. Can quaternion be used for that?
            // Quaternion multiplication perform rotation from right to left
            aPose.mOrientation = /*aDelta * */aMovement.mRotation * aPose.mOrientation;
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