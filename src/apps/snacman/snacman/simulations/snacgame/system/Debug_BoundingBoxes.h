#pragma once

#include "../component/GlobalPose.h"
#include "../component/LevelTags.h"
#include "../component/PlayerMoveState.h"
#include "../component/VisualModel.h"

#include <snacman/simulations/snacgame/GameContext.h>

#include <entity/Query.h>


namespace ad {
namespace snacgame {
namespace system {


class Debug_BoundingBoxes
{
public:
    Debug_BoundingBoxes(GameContext & aGameContext) :
        mPlayers{aGameContext.mWorld},
        mPills{aGameContext.mWorld}
    {}

    void update();

private:
    ent::Query<component::GlobalPose, component::PlayerMoveState, component::VisualModel> mPlayers;
    ent::Query<component::GlobalPose, component::Pill, component::VisualModel> mPills;
};

} // namespace system
} // namespace snacgame
} // namespace ad
