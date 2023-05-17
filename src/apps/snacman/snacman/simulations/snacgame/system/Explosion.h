#pragma once

#include "entity/Query.h"
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
namespace ad {
namespace snacgame {
namespace system {
class Explosion
{
public:
    Explosion(GameContext & aGameContext) :
        mExplosions(aGameContext.mWorld)
    {}

    void update(const snac::Time & aTime);
private:
    ent::Query<component::Explosion, component::Geometry> mExplosions;
};
} // namespace system
} // namespace snacgame
} // namespace ad
