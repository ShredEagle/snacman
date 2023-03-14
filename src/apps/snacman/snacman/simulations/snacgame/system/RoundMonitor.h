#pragma once

#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/component/PlayerLifeCycle.h"
#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

class RoundMonitor
{
public:
    RoundMonitor(ent::EntityManager & aWorld) :
        mPlayers{aWorld},
        mPills{aWorld},
        mLevel{aWorld},
        mLevelEntities{aWorld}
    {}

    void update();
private:
        ent::Query<component::PlayerLifeCycle> mPlayers;
        ent::Query<component::Pill>
            mPills;
        ent::Query<component::LevelData, component::LevelCreated> mLevel;
        ent::Query<component::LevelEntity> mLevelEntities;
};

} // namespace system
} // namespace snacgame
} // namespace ad
