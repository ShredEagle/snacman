#include "PlayerSpawner.h"

#include "../component/Controller.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerRoundData.h"
#include "../component/SceneNode.h"
#include "../component/GlobalPose.h"
#include "../component/Spawner.h"

#include "SceneGraphResolver.h"

#include "../Entities.h"
#include "../GameContext.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <math/Vector.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {

PlayerSpawner::PlayerSpawner(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayerSlots{mGameContext->mWorld},
    mPlayers{mGameContext->mWorld},
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

    std::vector<std::pair<EntHandle, component::PlayerRoundData *>> leaders;
    int leaderScore = 1;
    mPlayers.each([&leaders, &leaderScore](EntHandle aHandle, component::PlayerRoundData & aRoundData) {
        component::PlayerGameData & gameData = snac::getComponent<component::PlayerGameData>(aRoundData.mSlot);
        if (!aRoundData.mCrown.isValid())
        {
            if (gameData.mRoundsWon > leaderScore)
            {
                leaders.clear();
                leaders.push_back({aHandle, &aRoundData});
                leaderScore = gameData.mRoundsWon;
            }
            else if (gameData.mRoundsWon == leaderScore)
            {
                leaders.push_back({aHandle, &aRoundData});
            }
        }
    });

    for (auto & [leaderEnt, leaderData] : leaders)
    {
        EntHandle crown = createCrown(*mGameContext);
        leaderData->mCrown = crown;
        insertEntityInScene(crown, leaderEnt);
        updateGlobalPosition(snac::getComponent<component::SceneNode>(crown));
    }
}

} // namespace system
} // namespace snacgame
} // namespace ad
