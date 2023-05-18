#include "RoundMonitor.h"

#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../typedef.h"

#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

void RoundMonitor::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    if (mPills.countMatches() == 0)
    {
        ent::Phase destroyLevel;
        mLevelEntities.each([&destroyLevel](ent::Handle<ent::Entity> aHandle) {
            aHandle.get(destroyLevel)->erase();
        });

        // Tearing down level
        EntHandle level = *mGameContext->mLevel;
        component::LevelData & levelData =
            level.get(destroyLevel)->get<component::LevelData>();
        levelData.mSeed++; // update seed
        levelData.mTiles.clear(); // removes tiles
        levelData.mNodes.clear(); // removes pathfinding nodes
        levelData.mPortalIndex.clear(); // removes portal information
        /* levelData.mFile = */
        /*     mPlayers.countMatches() == 4 ? "snaclvl4.xml" : "snaclvl3.xml"; // choose right file */
        levelData.mFile =
            "snaclvl4.xml"; // choose right file
        level.get(destroyLevel)->remove<component::LevelCreated>();
        level.get(destroyLevel)->add(component::LevelToCreate{});

        // Removing players
        std::pair<std::vector<component::PlayerLifeCycle *>, int/*score*/> winners;
        winners.second = -1;

        mPlayers.each(
            [&destroyLevel, &winners]
            (EntHandle aHandle, component::PlayerLifeCycle & lifeCycle) 
            {
                if(winners.second < lifeCycle.mScore)
                {
                    winners.first.clear();
                    winners.first.push_back(&lifeCycle);
                    winners.second = lifeCycle.mScore;
                }
                else if(winners.second == lifeCycle.mScore)
                {
                    winners.first.push_back(&lifeCycle);
                }

                removeRoundTransientPlayerComponent(destroyLevel, aHandle);
                removeEntityFromScene(aHandle);
                // Notably reset alive status so that player can be spawned
                lifeCycle.resetBetweenRound();
            });

        for(auto & lifeCycle : winners.first)
        {
            ++(lifeCycle->mRoundsWon);
        }
    };
}

} // namespace system
} // namespace snacgame
} // namespace ad
