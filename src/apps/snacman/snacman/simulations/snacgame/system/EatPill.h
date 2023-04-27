#pragma once

#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "../GameContext.h"

#include "../component/Collision.h"
#include "../component/Geometry.h"
#include "../component/Text.h"
#include "../component/LevelTags.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerSlot.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class EatPill
{
public:
    EatPill(GameContext & aGameContext) :
        mPlayers{aGameContext.mWorld}, mPills{aGameContext.mWorld}
    {}

    void update();

private:
    ent::Query<component::GlobalPose,
               component::PlayerSlot,
               component::Collision,
               component::PlayerLifeCycle,
               component::Text>
        mPlayers;
    ent::Query<component::GlobalPose, component::Collision, component::Pill>
        mPills;
};

} // namespace system
} // namespace snacgame
} // namespace ad
