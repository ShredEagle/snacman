#pragma once

#include "entity/Entity.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/Spawner.h"
#include "../component/Geometry.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class PlayerSpawner
{
public:
    PlayerSpawner(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mSpawnable{mGameContext->mWorld},
        mSpawner{mGameContext->mWorld}
    {}

    void update(float aDelta);
private:
    GameContext * mGameContext;
    ent::Query<component::PlayerLifeCycle, component::Geometry> mSpawnable;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
