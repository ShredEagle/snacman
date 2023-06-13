#pragma once

#include <snacman/Timing.h>

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;

namespace component {
struct Explosion;
struct Geometry;
}

namespace system {
class Explosion
{
public:
    Explosion(GameContext & aGameContext);

    void update(const snac::Time & aTime);
private:
    ent::Query<component::Explosion, component::Geometry> mExplosions;
};
} // namespace system
} // namespace snacgame
} // namespace ad
