#pragma once

#include "../GameContext.h"

#include "../component/PlayerMoveState.h"
#include "../component/PlayerModel.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class AnimationManager
{
public:
    AnimationManager(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mAnimated{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerModel,
               component::PlayerMoveState>
        mAnimated;
};

} // namespace system
} // namespace snacgame
} // namespace ad
