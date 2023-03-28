#pragma once

#include "entity/Entity.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/Spawner.h"
#include "../component/Geometry.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class PlayerSpawner
{
public:
    PlayerSpawner(ent::EntityManager & aWorld, ent::Handle<ent::Entity> aLevel) :
        mSpawnable{aWorld},
        mSpawner{aWorld},
        mLevel{aLevel}
    {}

    void update(float aDelta);
private:
        ent::Query<component::PlayerLifeCycle, component::Geometry> mSpawnable;
        ent::Query<component::Spawner> mSpawner;
        ent::Handle<ent::Entity> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
