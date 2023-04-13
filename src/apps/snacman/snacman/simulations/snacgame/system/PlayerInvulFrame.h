#pragma once

#include "../component/PlayerLifeCycle.h"
#include "../component/VisualMesh.h"
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
    ent::Query<component::PlayerLifeCycle, component::VisualModel> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
