#pragma once


#include <entity/EntityManager.h>
#include <entity/Query.h>


namespace ad {

namespace snac {
struct Time;
}

namespace snacgame {

struct GameContext;

namespace component {
struct RigAnimation;
}

namespace system {

class AdvanceAnimations
{
public:
    AdvanceAnimations(GameContext & aGameContext);
    void update(const snac::Time & aSimulationTime);

private:
    ent::Query<component::RigAnimation> mAnimations;
};


} // namespace system
} // namespace snacgame
} // namespace ad

