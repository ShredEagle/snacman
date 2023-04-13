#include "PlayerInvulFrame.h"

#include "../component/VisualMesh.h"

#include <snac-renderer/Mesh.h>
#include <memory>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {

const std::shared_ptr<snac::Model> nullModel = std::make_shared<snac::Model>();

void PlayerInvulFrame::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mPlayer.each([aDelta, this](component::PlayerLifeCycle & aPlayer,
                                   component::VisualModel & aPlayerModel) {
        // TODO: (franz): this should be better
        if (aPlayer.mIsAlive && aPlayer.mInvulFrameCounter > 0.f && aPlayer.mHitStun <= 0.f) {
            aPlayer.mInvulFrameCounter -= aDelta;

            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 0)
            {
                aPlayerModel.mModel = mGameContext->mResources.getModel("models/donut/DONUT.gltf");
            }
            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 2)
            {
                aPlayerModel.mModel = nullModel;
            }
        }
        if (aPlayer.mIsAlive && aPlayer.mInvulFrameCounter > 0.f && aPlayer.mHitStun > 0.f)
        {
            aPlayer.mInvulFrameCounter -= aDelta;
            aPlayer.mHitStun -= aDelta;

            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 0)
            {
                aPlayerModel.mModel = mGameContext->mResources.getModel("models/donut/DONUT.gltf");
            }
            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 2)
            {
                aPlayerModel.mModel = nullModel;
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
