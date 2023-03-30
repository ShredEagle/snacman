#pragma once

#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/PathToOnGrid.h"
#include "../component/PlayerMoveState.h"

#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/AllowedMovement.h"
#include "../InputCommandConverter.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class ConsolidateGridMovement
{
public:
    ConsolidateGridMovement(ent::EntityManager & aWorld) :
        mPlayer{aWorld}, mPathfinder{aWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::AllowedMovement, component::Controller, component::PlayerMoveState>
        mPlayer;
    ent::Query<component::AllowedMovement, component::Geometry, component::PathToOnGrid>
        mPathfinder;
};

} // namespace system
} // namespace snacgame
} // namespace ad
