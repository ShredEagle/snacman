#include "PlayerSpawner.h"

#include "../component/Spawner.h"
#include "../Entities.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <math/Vector.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

void PlayerSpawner::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    mSpawnable.each([this](EntHandle aPlayerHandle,
                           component::PlayerLifeCycle & aPlayer,
                           component::PlayerMoveState & aMoveState,
                           component::Geometry & aPlayerGeometry) {
        if (!aPlayer.mIsAlive)
        {
            // TODO: Needs an alg to choose the right spawner if there are
            // many spawner
            mSpawner.each([this, &aPlayerHandle, &aPlayer, &aMoveState,
                           &aPlayerGeometry]
                           (component::Spawner & aSpawner) {
                if (!aPlayer.mIsAlive && !aSpawner.mSpawnedPlayer)
                {
                    aPlayer.mIsAlive = true;
                    aPlayer.mTimeToRespawn = component::gBaseTimeToRespawn;
                    aPlayer.mInvulFrameCounter =
                        component::gBaseInvulFrameDuration;
                    aPlayerGeometry.mPosition = aSpawner.mSpawnPosition;
                    aSpawner.mSpawnedPlayer = true;
                    aMoveState.mMoveState = gPlayerMoveFlagNone;
                    insertEntityInScene(aPlayerHandle, *mGameContext->mLevel);
                }
            });
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
