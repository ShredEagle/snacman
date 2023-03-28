#include "PlayerSpawner.h"

#include "../component/Spawner.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <snacman/Profiling.h>

#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace system {

void PlayerSpawner::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mSpawnable.each([this, aDelta, mLevel = mLevel](EntHandle aPlayerHandle, component::PlayerLifeCycle & aPlayer,
                                   component::Geometry & aPlayerGeometry) {
        if (!aPlayer.mIsAlive) {
            if (aPlayer.mTimeToRespawn < 0) {
                aPlayer.mTimeToRespawn -= aDelta;
            } else {
                // TODO: Needs an alg to choose the right spawner if there are
                // many spawner
                mSpawner.each([mLevel, aPlayerHandle, &aPlayer, &aPlayerGeometry](component::Spawner aSpawner) {
                    if (!aPlayer.mIsAlive) {
                        aPlayer.mIsAlive = true;
                        aPlayer.mTimeToRespawn = component::gBaseTimeToRespawn;
                        aPlayer.mInvulFrameCounter = component::gBaseInvulFrameDuration;
                        aPlayerGeometry.mPosition = aSpawner.mSpawnPosition;
                        insertEntityInScene(aPlayerHandle, mLevel);
                    }
                });
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
