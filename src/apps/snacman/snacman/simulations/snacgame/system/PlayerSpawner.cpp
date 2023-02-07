#include "PlayerSpawner.h"
#include "math/Vector.h"

#include "../component/Spawner.h"

#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

void PlayerSpawner::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

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
                        aPlayerGeometry.mGridPosition = aSpawner.mSpawnPosition;
                        aPlayerGeometry.mSubGridPosition = math::Position<2, float>::Zero();
                    }
                });
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
