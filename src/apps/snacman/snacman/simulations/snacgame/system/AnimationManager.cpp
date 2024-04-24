#include "AnimationManager.h"

#include "../component/RigAnimation.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

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

        if (newAnimName != playerAnim.mAnimation->mName)
        {
            playerAnim.mAnimation = &playerAnim.mAnimatedRig->mNameToAnimation.at(newAnimName);
            playerAnim.mStartTime = snac::Clock::now();
            playerAnim.mParameter = decltype(component::RigAnimation::mParameter){
                playerAnim.mAnimation->mDuration,
                animSpeed
            };
        }
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
