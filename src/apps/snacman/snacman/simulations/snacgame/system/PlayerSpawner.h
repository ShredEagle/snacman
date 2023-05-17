#pragma once

#include "../GameContext.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerMoveState.h"
#include "../component/PlayerSlot.h"
#include "../component/Spawner.h"
#include "../component/Geometry.h"

#include <entity/Entity.h>
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
    ent::Query<component::PlayerLifeCycle,
               component::PlayerSlot,
               component::Geometry,
               component::PlayerMoveState> mSpawnable;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
