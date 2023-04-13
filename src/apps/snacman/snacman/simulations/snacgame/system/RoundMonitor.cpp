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

        EntHandle level = *mGameContext->mLevel;
        component::LevelData & levelData =
            level.get(destroyLevel)->get<component::LevelData>();
        // TODO: (franz): make a function for the level teardown
        // before the round change
        levelData.mSeed++;
        levelData.mTiles.clear();
        levelData.mNodes.clear();
        levelData.mFile =
            mPlayers.countMatches() == 4 ? "snaclvl4.xml" : "snaclvl3.xml";
        level.get(destroyLevel)->remove<component::LevelCreated>();
        level.get(destroyLevel)->add(component::LevelToCreate{});
        mPlayers.each([&destroyLevel](EntHandle aHandle,
                                      component::PlayerLifeCycle & lifeCycle,
                                      component::PlayerPowerUp & aPowerup) {
            lifeCycle.mIsAlive = false;
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
