
#pragma once

#include "../GameContext.h"
#include "../component/Geometry.h"
#include "../component/LevelTags.h"
#include "../component/PlayerSlot.h"
#include "../component/Collision.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class PowerUpPickup
{
public:
    PowerUpPickup(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mPlayers{mGameContext->mWorld},
        mPowerups{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::Geometry, component::PlayerSlot, component::Collision> mPlayers;
    ent::Query<component::Geometry, component::PowerUp, component::Collision, component::LevelEntity> mPowerups;
};

} // namespace system
} // namespace snacgame
} // namespace ad
