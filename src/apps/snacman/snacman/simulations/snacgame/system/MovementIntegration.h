#pragma once

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
    ent::Query<component::Geometry, component::Speed> mMovables;
    ent::Query<component::MovementScreenSpace, component::PoseScreenSpace>
        mScreenMovables;
};

} // namespace system
} // namespace snacgame
} // namespace ad
