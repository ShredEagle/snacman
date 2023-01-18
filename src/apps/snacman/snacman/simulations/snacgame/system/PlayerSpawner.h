#pragma once

#include "../component/PlayerLifeCycle.h"
#include "../component/Spawner.h"
#include "../component/Geometry.h"

#include "entity/EntityManager.h"
#include "entity/Query.h"

namespace ad {
namespace snacgame {
namespace system {

class PlayerSpawner
{
public:
    PlayerSpawner(ent::EntityManager & aWorld) :
        mSpawnable{aWorld},
        mSpawner{aWorld}
    {}

    void update(float aDelta);
private:
        ent::Query<component::PlayerLifeCycle, component::Geometry> mSpawnable;
        ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
