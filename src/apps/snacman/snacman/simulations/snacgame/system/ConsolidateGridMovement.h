#pragma once

#include "../GameContext.h"

#include "../component/PathToOnGrid.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerMoveState.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/LevelData.h"
#include "../component/AllowedMovement.h"
#include "../InputCommandConverter.h"

#include <snacman/Input.h>

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class ConsolidateGridMovement
{
public:
    ConsolidateGridMovement(GameContext & aGameContext) :
        mPlayer{aGameContext.mWorld}, mPathfinder{aGameContext.mWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::AllowedMovement, component::Controller, component::PlayerMoveState, component::PlayerLifeCycle>
        mPlayer;
    ent::Query<component::AllowedMovement, component::Geometry, component::PathToOnGrid, component::GlobalPose>
        mPathfinder;
};

} // namespace system
} // namespace snacgame
} // namespace ad
