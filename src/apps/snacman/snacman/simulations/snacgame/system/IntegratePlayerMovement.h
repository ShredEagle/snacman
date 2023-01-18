#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"

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
        mPlayer{aWorld}, mLevel(aLevel)
    {}

    void update(float aDelta);

private:
    ent::Query<component::Geometry, component::PlayerMoveState> mPlayer;
    ent::Handle<ent::Entity> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
