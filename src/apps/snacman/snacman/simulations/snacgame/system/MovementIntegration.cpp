#include "MovementIntegration.h"

namespace ad {
namespace snacgame {
namespace system {


void MovementIntegration::update(float aDelta)
{
    ent::Phase integrate;
    mScreenMovable.each(
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
