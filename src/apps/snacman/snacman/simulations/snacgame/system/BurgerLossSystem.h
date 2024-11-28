#pragma once

#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/Physics.h"
#include "snacman/simulations/snacgame/component/Tags.h"
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class BurgerLoss
{
public:
    BurgerLoss(GameContext & aGameContext);

    void update(ent::Handle<ent::Entity> aLevel);

private:
    ent::Query<component::BurgerLossHitbox, component::Collision, component::GlobalPose>
        mBurgerHitbox;
    ent::Query<component::BurgerParticle, component::Collision, component::GlobalPose>
        mBurgerParticles;
    GameContext * mContext;
};

} // namespace system
} // namespace snacgame
} // namespace ad
