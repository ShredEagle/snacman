#pragma once

#include "../component/PlayerSlot.h"
#include "../component/Collision.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/PlayerMoveState.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class EatPill
{
public:
    EatPill(ent::EntityManager & aWorld,
                            ent::Handle<ent::Entity> aLevel) :
        mPlayers{aWorld}, mPills{aWorld}
    {}

    void update();

private:
    ent::Query<component::Geometry, component::PlayerSlot, component::Collision> mPlayers;
    ent::Query<component::Geometry, component::Collision, component::Pill> mPills;
};

} // namespace system
} // namespace snacgame
} // namespace ad
