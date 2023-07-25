#include "PlayerSpawner.h"

#include "SceneGraphResolver.h"
#include "snacman/simulations/snacgame/component/Controller.h"

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
    mPlayerSlots{mGameContext->mWorld},
    mSpawner{mGameContext->mWorld}
{}

void PlayerSpawner::spawnPlayers(EntHandle aLevel)
{
    TIME_RECURRING_CLASSFUNC(Main);

    Phase spawnPlayer;
    std::vector<EntHandle> mSpawnedSlots;
    mPlayerSlots.each(
        [this, &aLevel, &mSpawnedSlots](EntHandle aSlotHandle, const component::PlayerSlot &)
        {
        component::PlayerSlot & slot = snac::getComponent<component::PlayerSlot>(aSlotHandle);
        EntHandle spawnHandle = snac::getFirstHandle(
            mSpawner, [](const component::Spawner & aSpawner)
            { return !aSpawner.mSpawnedPlayer; });
        if (spawnHandle.isValid() && !slot.mPlayer.isValid())
        {
            component::Spawner & spawner = snac::getComponent<component::Spawner>(spawnHandle);
            slot.mPlayer = createInGamePlayer(*mGameContext, aSlotHandle, spawner.mSpawnPosition);
            insertEntityInScene(slot.mPlayer, aLevel);
            updateGlobalPosition(
                snac::getComponent<component::SceneNode>(slot.mPlayer));
            spawner.mSpawnedPlayer = true;
            mSpawnedSlots.push_back(spawnHandle);
        }
    });

    for (EntHandle spawnHandle : mSpawnedSlots)
    {
        spawnHandle.get(spawnPlayer)->remove<component::Controller>();
    }
}

} // namespace system
} // namespace snacgame
} // namespace ad
