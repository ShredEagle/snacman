#pragma once

#include "../GameContext.h"
#include "../component/LevelData.h"
#include "../component/PlayerLifeCycle.h"

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

class RoundMonitor
{
public:
    RoundMonitor(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mPlayers{mGameContext->mWorld},
        mPills{mGameContext->mWorld},
        mLevelEntities{mGameContext->mWorld}
    {}

    void update();
private:
    GameContext * mGameContext;
    ent::Query<component::PlayerLifeCycle> mPlayers;
    ent::Query<component::Pill>
        mPills;
    ent::Query<component::LevelEntity> mLevelEntities;
};

} // namespace system
} // namespace snacgame
} // namespace ad
