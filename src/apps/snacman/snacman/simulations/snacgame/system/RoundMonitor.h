#pragma once

#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/component/PlayerLifeCycle.h"
#include "snacman/simulations/snacgame/component/PlayerPowerUp.h"
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
    ent::Query<component::PlayerLifeCycle, component::PlayerPowerUp> mPlayers;
    ent::Query<component::Pill>
        mPills;
    ent::Query<component::LevelEntity> mLevelEntities;
};

} // namespace system
} // namespace snacgame
} // namespace ad
