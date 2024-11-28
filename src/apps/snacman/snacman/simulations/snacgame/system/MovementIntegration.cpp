#include "MovementIntegration.h"

#include "math/Vector.h"
#include "snacman/simulations/snacgame/component/Tags.h"
#include "snacman/simulations/snacgame/Entities.h"

#include "../component/Geometry.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"
#include "../component/PoseScreenSpace.h"
#include "../component/Speed.h"
#include "../GameContext.h"

#include <entity/EntityManager.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

MovementIntegration::MovementIntegration(GameContext & aGameContext) :
    mGravityObject{aGameContext.mWorld},
    mMovables{aGameContext.mWorld},
    mScreenMovables{aGameContext.mWorld},
    mBurgerParticles{aGameContext.mWorld}
{}

void MovementIntegration::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase integrate;
    mGravityObject.each([aDelta](component::Speed & aMovement,
                                 const component::Gravity & aGravity) {
        aMovement.mSpeed.z() += aGravity.mGravityAccel * aDelta;
    });
    mBurgerParticles.each([aDelta](component::Speed & aSpeed,
                                   const component::BurgerParticle & aBurger,
                                   const component::Geometry & aGeo) {
        aSpeed.mSpeed = aSpeed.mSpeed
                        + math::Vec<3, float>{
                            0.f, 0.f,
                            -2.f * gBurgerHeightLaunch * aBurger.mBaseSpeed
                                * aBurger.mBaseSpeed * aDelta
                                / (aBurger.mTargetNorm * aBurger.mTargetNorm)};
    });
    mMovables.each([aDelta](const component::Speed & aMovement,
                            component::Geometry & aPose) {
        math::Quaternion rotation{aMovement.mRotation.mAxis,
                                  aDelta * aMovement.mRotation.mAngle};
        // Quaternion multiplication perform rotation from right to left
        aPose.mOrientation = rotation * aPose.mOrientation;
        aPose.mPosition += aDelta * aMovement.mSpeed;
    });
    mScreenMovables.each(
        [aDelta](const component::MovementScreenSpace & aMovement,
                 component::PoseScreenSpace & aPose) {
            aPose.mRotationCCW += aDelta * aMovement.mAngularSpeed;
        });
}

} // namespace system
} // namespace snacgame
} // namespace ad
