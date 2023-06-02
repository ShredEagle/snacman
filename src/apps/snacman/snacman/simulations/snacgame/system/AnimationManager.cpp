#include "AnimationManager.h"

#include "../component/RigAnimation.h"
#include "../component/PlayerRoundData.h"

#include "../GameContext.h"
#include "../InputConstants.h"
#include "../typedef.h"

#include <entity/EntityManager.h>
#include <string>

namespace ad {
namespace snacgame {
namespace system {
AnimationManager::AnimationManager(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mAnimated{mGameContext->mWorld}
{}

void AnimationManager::update()
{
    mAnimated.each([](component::PlayerRoundData & aRoundData) {
        Phase animPhase;
        float animSpeed = 0.6f;
        std::string newAnimName = "idle";
        if (aRoundData.mMoveState
            & (gPlayerMoveFlagUp | gPlayerMoveFlagDown | gPlayerMoveFlagRight
               | gPlayerMoveFlagLeft))
        {
            newAnimName = "run";
            animSpeed = 1.3f;
        }

        component::RigAnimation & playerAnim = aRoundData.mModel.get(animPhase)->get<component::RigAnimation>();

        if (newAnimName != playerAnim.mAnimName)
        {
            playerAnim.mAnimation = &playerAnim.mAnimationMap->at(newAnimName);
            playerAnim.mAnimName = newAnimName;
            playerAnim.mStartTime = snac::Clock::now();
            playerAnim.mParameter = decltype(component::RigAnimation::mParameter){
                playerAnim.mAnimation->mEndTime,
                animSpeed
            };
        }
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
