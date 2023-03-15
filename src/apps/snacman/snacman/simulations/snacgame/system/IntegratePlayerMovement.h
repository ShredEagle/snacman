#pragma once

#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/PlayerMoveState.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class IntegratePlayerMovement
{
public:
    IntegratePlayerMovement(ent::EntityManager & aWorld) :
        mPlayer{aWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::Geometry, component::PlayerMoveState> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
