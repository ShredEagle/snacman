#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"
#include "snacman/simulations/snacgame/component/LevelData.h"

#include "../component/Geometry.h"
#include "../component/PlayerMoveState.h"

namespace ad {
namespace snacgame {
namespace system {

class IntegratePlayerMovement
{
public:
    IntegratePlayerMovement(ent::EntityManager & aWorld,
                            ent::Handle<ent::Entity> aLevel) :
        mPlayer{aWorld}, mLevel{aWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::Geometry, component::PlayerMoveState> mPlayer;
    ent::Query<component::LevelData, component::LevelCreated> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
