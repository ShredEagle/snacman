#include "PlayerInvulFrame.h"

namespace ad {
namespace snacgame {
namespace system {

void PlayerInvulFrame::update(float aDelta)
{
    mPlayer.each([aDelta](component::PlayerLifeCycle & aPlayer,
                                   component::Geometry & aPlayerGeometry) {
        if (aPlayer.mIsAlive && aPlayer.mInvulFrameCounter > 0) {
            aPlayer.mInvulFrameCounter -= aDelta;

            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 0)
            {
                aPlayerGeometry.mColor.a() = 1.f;
            }
            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 2)
            {
                aPlayerGeometry.mColor.a() = 0.f;
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
