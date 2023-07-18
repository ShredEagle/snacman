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
    mSpawnedPlayers{mGameContext->mWorld},
    mSpawner{mGameContext->mWorld}
{}

void PlayerSpawner::despawnPlayers()
{
    TIME_RECURRING_CLASSFUNC(Main);
    Phase despawnPlayer;
    
    mSpawnedPlayers.each(
        [&despawnPlayer](EntHandle aSlotHandle, const component::PlayerSlot &)
        {
            aSlotHandle.get(despawnPlayer)->add(component::Unspawned());
        }
    );
}

struct PlayerToSpawn {
    EntHandle mSlot;
    Pos3 mPosition;
};

void PlayerSpawner::spawnPlayers(EntHandle aLevel)
{
    TIME_RECURRING_CLASSFUNC(Main);

    Phase spawnPlayer;
    std::vector<PlayerToSpawn> playersToSpawn;
    mUnspawnedPlayers.each(
        [this, &playersToSpawn](EntHandle aSlotHandle, const component::PlayerSlot &)
        {
        EntHandle spawnHandle = snac::getFirstHandle(
            mSpawner, [](const component::Spawner & aSpawner)
            { return !aSpawner.mSpawnedPlayer; });
        if (spawnHandle.isValid())
        {
            component::Spawner & spawner =
                spawnHandle.get()->get<component::Spawner>();
            playersToSpawn.push_back({aSlotHandle, spawner.mSpawnPosition});
            spawner.mSpawnedPlayer = true;
        }
    });

    for (auto aPlayerToSpawn : playersToSpawn)
    {
            EntHandle slotHandle = aPlayerToSpawn.mSlot;
            Pos3 spawnPosition = aPlayerToSpawn.mPosition;
            EntHandle playerHandle = createInGamePlayer(
                *mGameContext, slotHandle,
                spawnPosition);
            insertEntityInScene(playerHandle, aLevel);
            updateGlobalPosition(
                playerHandle.get()->get<component::SceneNode>());
            
            //Remove player from the spawnable pool
            slotHandle.get(spawnPlayer)->remove<component::Unspawned>();

    }
}

} // namespace system
} // namespace snacgame
} // namespace ad
