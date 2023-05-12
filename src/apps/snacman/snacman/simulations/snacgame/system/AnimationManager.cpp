#include "AnimationManager.h"

#include "snacman/simulations/snacgame/component/PlayerMoveState.h"
#include "snacman/simulations/snacgame/component/RigAnimation.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "../typedef.h"
#include <string>

namespace ad {
namespace snacgame {
namespace system {

void AnimationManager::update()
{
    mAnimated.each([](component::PlayerModel & aModel, const component::PlayerMoveState & aMoveState) {
        Phase animPhase;
        std::string newAnimName = "idle";
        if (aMoveState.mMoveState
            & (gPlayerMoveFlagUp | gPlayerMoveFlagDown | gPlayerMoveFlagRight
               | gPlayerMoveFlagLeft))
        {
            newAnimName = "run";
        }

        component::RigAnimation & playerAnim = aModel.mModel.get(animPhase)->get<component::RigAnimation>();

        if (newAnimName != playerAnim.mAnimName)
        {
            playerAnim.mAnimation = &playerAnim.mAnimationMap->at(newAnimName);
            playerAnim.mAnimName = newAnimName;
            playerAnim.mStartTime = snac::Clock::now();
            playerAnim.mParameter = decltype(component::RigAnimation::mParameter){playerAnim.mAnimation->mEndTime};
        }
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
