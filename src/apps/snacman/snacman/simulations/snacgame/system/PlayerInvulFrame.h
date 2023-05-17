#pragma once

#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/VisualModel.h"
#include "../GameContext.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class PlayerInvulFrame
{
public:
    PlayerInvulFrame(GameContext & aGameContext) :
        mGameContext{&aGameContext}, mPlayer{mGameContext->mWorld}
    {}

    void update(float aDelta);

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerLifeCycle, component::PlayerModel> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
