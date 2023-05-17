#include "PlayerSpawner.h"

#include "../Entities.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include "../component/Spawner.h"

#include <snacman/Profiling.h>

#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace system {

void PlayerSpawner::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mSpawnable.each([this, &aDelta](EntHandle aPlayerHandle,
                                    component::PlayerLifeCycle & aPlayer,
                                    component::PlayerMoveState & aMoveState,
                                    component::PlayerSlot & aSlot,
                                    component::Geometry & aPlayerGeometry) 
    {
        if (!aPlayer.mIsAlive) {
            if (aPlayer.mTimeToRespawn < 0) {
                aPlayer.mTimeToRespawn -= aDelta;
            } else {
                // TODO: Needs an alg to choose the right spawner if there are
                // many spawner
                mSpawner.each([this, aPlayerHandle, &aPlayer, &aMoveState, &aSlot, &aPlayerGeometry]
                              (component::Spawner & aSpawner) 
                {
                    if (!aPlayer.mIsAlive && !aSpawner.mSpawnedPlayer) 
                    {
                        aPlayer.mIsAlive = true;
                        aPlayer.mTimeToRespawn = component::gBaseTimeToRespawn;
                        aPlayer.mInvulFrameCounter = component::gBaseInvulFrameDuration;
                        aPlayer.mHud = createHudBillpad(*mGameContext, aSlot);
                        aPlayerGeometry.mPosition = aSpawner.mSpawnPosition;
                        aSpawner.mSpawnedPlayer = true;
                        aMoveState.mMoveState = gPlayerMoveFlagNone;
                        insertEntityInScene(aPlayerHandle, *mGameContext->mLevel);
                    }
                });
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
