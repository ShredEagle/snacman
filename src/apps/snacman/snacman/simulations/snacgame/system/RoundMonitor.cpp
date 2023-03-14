#include "RoundMonitor.h"
#include "snacman/simulations/snacgame/component/LevelData.h"

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
        mLevel.each([&destroyLevel](ent::Handle<ent::Entity> aHandle, component::LevelData & aLevelData) {
                aLevelData.mSeed++;
                aLevelData.mTiles.clear();
                aHandle.get(destroyLevel)->remove<component::LevelCreated>();
                aHandle.get(destroyLevel)->add(component::LevelToCreate{});
        });
        mPlayers.each([](component::PlayerLifeCycle & lifeCycle) {
                lifeCycle.mIsAlive = false;
        });
    };
}

} // namespace system
} // namespace snacgame
} // namespace ad
