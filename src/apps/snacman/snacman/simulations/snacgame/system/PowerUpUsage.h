#pragma once

#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/PlayerHud.h"
#include "snacman/simulations/snacgame/component/Speed.h"
#include "snacman/simulations/snacgame/component/VisualModel.h"

#include "../component/Collision.h"
#include "../component/Controller.h"
#include "../component/LevelTags.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerPowerUp.h"
#include "../component/PlayerSlot.h"
#include "../component/PowerUp.h"
#include "../GameContext.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <math/Vector.h>
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
        mInGameDogPowerups(mGameContext->mWorld),
        mInGameMissilePowerups(mGameContext->mWorld)
    {}

    void update(float aDelta);

    std::pair<math::Position<2, float>, ent::Handle<ent::Entity>>
    getDogPlacementTile(ent::Handle<ent::Entity> aHandle,
                            const component::Geometry & aPlayerGeo);
    std::optional<ent::Handle<ent::Entity>>
    getClosestPlayer(ent::Handle<ent::Entity> aHandle,
                     const math::Position<3, float> & aPos);

private:
    GameContext * mGameContext;
    ent::Query<component::GlobalPose,
               component::Geometry,
               component::PlayerSlot,
               component::Collision,
               component::PlayerLifeCycle>
        mPlayers;
    ent::Query<component::Geometry,
               component::PlayerPowerUp,
               component::PlayerSlot,
               component::GlobalPose,
               component::Controller>
        mPowUpPlayers;
    ent::Query<component::GlobalPose,
               component::Geometry,
               component::PowerUp,
               component::Collision,
               component::VisualModel,
               component::LevelEntity>
        mPowerups;
    ent::Query<component::GlobalPose,
               component::InGamePowerup,
               component::Collision,
               component::Geometry,
               component::LevelEntity>
        mInGameDogPowerups;
    ent::Query<component::GlobalPose,
               component::InGamePowerup,
               component::Speed,
               component::Geometry,
               component::LevelEntity>
        mInGameMissilePowerups;
};

} // namespace system
} // namespace snacgame
} // namespace ad
