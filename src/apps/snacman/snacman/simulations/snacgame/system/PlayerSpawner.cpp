#include "PlayerSpawner.h"

#include "../component/Spawner.h"

namespace ad {
namespace snacgame {
namespace system {

void PlayerSpawner::update(float aDelta)
{
    mSpawnable.each([this, aDelta](component::PlayerLifeCycle & aPlayer,
                                   component::Geometry & aPlayerGeometry) {
        if (!aPlayer.mIsAlive) {
            if (aPlayer.mTimeToRespawn < 0) {
                aPlayer.mTimeToRespawn -= aDelta;
            } else {
                // TODO: Needs an alg to choose the right spawner if there are
                // many spawner
                mSpawner.each([&aPlayer, &aPlayerGeometry](component::Spawner aSpawner) {
                    if (!aPlayer.mIsAlive) {
                        aPlayer.mIsAlive = true;
                        aPlayer.mTimeToRespawn = component::gBaseTimeToRespawn;
                        aPlayer.mInvulFrameCounter = component::gBaseInvulFrameDuration;
                        aPlayerGeometry.mPosition = aSpawner.mSpawnPosition;
                        aPlayerGeometry.mShouldBeDrawn = true;
                    }
                });
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
