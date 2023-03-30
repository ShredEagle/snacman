#include "RoundMonitor.h"
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
        level.get(destroyLevel)->remove<component::LevelCreated>();
        level.get(destroyLevel)->add(component::LevelToCreate{});
        mPlayers.each([](component::PlayerLifeCycle & lifeCycle) {
                lifeCycle.mIsAlive = false;
        });
    };
}

} // namespace system
} // namespace snacgame
} // namespace ad
