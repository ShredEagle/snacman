#pragma once

#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "../GameContext.h"
#include "../component/Controller.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerPowerUp.h"
#include "../component/LevelTags.h"
#include "../component/PlayerSlot.h"
#include "../component/Collision.h"

#include <math/Vector.h>

#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {

class PowerUpUsage
{
public:
    PowerUpUsage(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mPlayers{mGameContext->mWorld},
        mPowUpPlayers{mGameContext->mWorld},
        mPowerups{mGameContext->mWorld},
        mInGamePowerups(mGameContext->mWorld)
    {}

    void update();

    std::pair<math::Position<2, float>, ent::Handle<ent::Entity>> getPowerupPlacementTile(ent::Handle<ent::Entity> aHandle, const component::Geometry & aPlayerGeo);

private:
    GameContext * mGameContext;
    ent::Query<component::GlobalPose, component::Geometry, component::PlayerSlot, component::Collision, component::PlayerLifeCycle> mPlayers;
    ent::Query<component::Geometry, component::PlayerSlot, component::PlayerPowerUp, component::Controller> mPowUpPlayers;
    ent::Query<component::GlobalPose, component::PowerUp, component::Collision, component::LevelEntity> mPowerups;
    ent::Query<component::GlobalPose, component::InGamePowerup, component::Collision, component::LevelEntity> mInGamePowerups;
};

} // namespace system
} // namespace snacgame
} // namespace ad
