#pragma once

#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/Physics.h"
#include "snacman/simulations/snacgame/component/Tags.h"
#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Geometry;
struct MovementScreenSpace;
struct PoseScreenSpace;
struct Speed;
}
namespace system {

class MovementIntegration
{
public:
    MovementIntegration(GameContext & aGameContext);

    void update(float aDelta);

private:
    ent::Query<component::Gravity, component::Speed, component::Geometry> mGravityObject;
    ent::Query<component::Geometry, component::Speed> mMovables;
    ent::Query<component::MovementScreenSpace, component::PoseScreenSpace>
        mScreenMovables;
    ent::Query<component::BurgerParticle, component::Speed, component::Geometry>
        mBurgerParticles;
};

} // namespace system
} // namespace snacgame
} // namespace ad
