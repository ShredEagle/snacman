#pragma once

#include "../GameContext.h"

#include "../component/Geometry.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PoseScreenSpace.h"
#include "../component/Speed.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class MovementIntegration
{
public:
    MovementIntegration(GameContext & aGameContext) :
        mMovables{aGameContext.mWorld}, mScreenMovables{aGameContext.mWorld}
    {}

    void update(float aDelta);

private:
    ent::Query<component::Geometry, component::Speed> mMovables;
    ent::Query<component::MovementScreenSpace, component::PoseScreenSpace>
        mScreenMovables;
};

} // namespace system
} // namespace snacgame
} // namespace ad
