#include "PlayerSpawner.h"

#include "SceneGraphResolver.h"

#include "../component/Spawner.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h"
#include "../component/SceneNode.h"

#include "../GameContext.h"
#include "../Entities.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <snacman/QueryManipulation.h>
#include <snacman/Profiling.h>

#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace system {

PlayerSpawner::PlayerSpawner(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayers{mGameContext->mWorld},
    mSpawner{mGameContext->mWorld}
{}

void PlayerSpawner::spawnPlayers()
{
    TIME_RECURRING_CLASSFUNC(Main);

    int playerJoinCount = mGameContext->mPlayerSlots.size();

    const int currentPlayerCount = mPlayers.countMatches();

    for (int index = currentPlayerCount; index < playerJoinCount; ++index)
    {
        OptEntHandle spawnHandle = snac::getFirstHandle(mSpawner, [](const component::Spawner & aSpawner) { return !aSpawner.mSpawnedPlayer;});
        if (spawnHandle)
        {
            const component::Spawner & spawner = spawnHandle->get()->get<component::Spawner>();
            EntHandle playerHandle = createInGamePlayer(*mGameContext, mGameContext->mPlayerSlots.at(index), spawner.mSpawnPosition);
            insertEntityInScene(playerHandle, *mGameContext->mLevel);
            updateGlobalPosition(playerHandle.get()->get<component::SceneNode>());
        }
    }
}

} // namespace system
} // namespace snacgame
} // namespace ad
