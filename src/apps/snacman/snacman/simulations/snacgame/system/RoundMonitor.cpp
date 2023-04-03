#include "RoundMonitor.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
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
        component::LevelData & levelData = level.get(destroyLevel)->get<component::LevelData>();
        levelData.mSeed++;
        levelData.mTiles.clear();
        levelData.mFile = mPlayers.countMatches() == 4 ? "snaclvl4.xml" : "snaclvl3.xml";
        level.get(destroyLevel)->remove<component::LevelCreated>();
        level.get(destroyLevel)->add(component::LevelToCreate{});
        mPlayers.each([](EntHandle aHandle, component::PlayerLifeCycle & lifeCycle) {
                lifeCycle.mIsAlive = false;
                removeEntityFromScene(aHandle);
        });
    };
}

} // namespace system
} // namespace snacgame
} // namespace ad
