#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"

#include "../component/MovementScreenSpace.h"
#include "../component/PoseScreenSpace.h"

namespace ad {
namespace snacgame {
namespace system {


class MovementIntegration
{
public:
    MovementIntegration(ent::EntityManager & aWorld) :
        mScreenMovable{aWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::MovementScreenSpace, component::PoseScreenSpace> mScreenMovable;
};


} // namespace system
} // namespace snacgame
} // namespace ad
