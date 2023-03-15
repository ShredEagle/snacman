#pragma once

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
    EatPill(ent::EntityManager & aWorld) :
        mPlayers{aWorld}, mPills{aWorld}
    {}

    void update();

private:
    ent::Query<component::Geometry,
               component::PlayerSlot,
               component::Collision,
               component::PlayerLifeCycle,
               component::Text>
        mPlayers;
    ent::Query<component::Geometry, component::Collision, component::Pill>
        mPills;
};

} // namespace system
} // namespace snacgame
} // namespace ad
