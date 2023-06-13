#include "PlayerSpawner.h"

#include "SceneGraphResolver.h"

#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"
#include "../component/SceneNode.h"
#include "../component/GlobalPose.h"
#include "../component/Spawner.h"
#include "../Entities.h"
#include "../GameContext.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <math/Vector.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>

namespace ad {
namespace snacgame {
namespace system {

PlayerSpawner::PlayerSpawner(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mUnspawnedPlayers{mGameContext->mWorld},
    mSpawner{mGameContext->mWorld}
{}

void PlayerSpawner::spawnPlayers()
{
    TIME_RECURRING_CLASSFUNC(Main);

    Phase spawnPlayer;
    mUnspawnedPlayers.each(
        [this, &spawnPlayer](EntHandle aSlotHandle, const component::PlayerSlot &)
        {
        OptEntHandle spawnHandle = snac::getFirstHandle(
            mSpawner, [](const component::Spawner & aSpawner)
            { return !aSpawner.mSpawnedPlayer; });
        if (spawnHandle)
        {
            component::Spawner & spawner =
                spawnHandle->get()->get<component::Spawner>();
            EntHandle playerHandle = createInGamePlayer(
                *mGameContext, aSlotHandle,
                spawner.mSpawnPosition);
            insertEntityInScene(playerHandle, *mGameContext->mLevel);
            updateGlobalPosition(
                playerHandle.get()->get<component::SceneNode>());
            
            //Remove player from the spawnable pool
            aSlotHandle.get(spawnPlayer)->remove<component::Unspawned>();

            spawner.mSpawnedPlayer = true;
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
