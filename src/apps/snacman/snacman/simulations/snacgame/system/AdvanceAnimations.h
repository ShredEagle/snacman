#pragma once

#include "../GameContext.h"

#include "../component/RigAnimation.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>


namespace ad {
namespace snacgame {
namespace system {

class AdvanceAnimations
{
public:
    AdvanceAnimations(GameContext & aGameContext) :
        mAnimations{aGameContext.mWorld}
    {}

    void update(const snac::Time & aTime);

private:
    ent::Query<component::RigAnimation> mAnimations;
};


} // namespace system
} // namespace snacgame
} // namespace ad

