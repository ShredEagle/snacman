#include "RoundMonitor.h"

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
        levelData.mFile =
            mPlayers.countMatches() == 4 ? "snaclvl4.xml" : "snaclvl3.xml"; // choose right file
        level.get(destroyLevel)->remove<component::LevelCreated>();
        level.get(destroyLevel)->add(component::LevelToCreate{});

        // Removing players
        mPlayers.each([&destroyLevel](EntHandle aHandle,
                                      component::PlayerLifeCycle & lifeCycle,
                                      component::PlayerPowerUp & aPowerup) {
            // Reset alive status so that player can be spawned
            lifeCycle.mIsAlive = false;
            lifeCycle.mTimeToRespawn = component::gBaseTimeToRespawn;

            // Removes powerup if the player has any
            if (aHandle.get()->has<component::PlayerPowerUp>())
            {
                aHandle.get(destroyLevel)
                    ->get<component::PlayerPowerUp>()
                    .mPowerUp.get(destroyLevel)
                    ->erase();
                aHandle.get(destroyLevel)->remove<component::PlayerPowerUp>();
            }
            removeEntityFromScene(aHandle);
        });
    };
}

} // namespace system
} // namespace snacgame
} // namespace ad
