#include "PlayerInvulFrame.h"

#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/RigAnimation.h"

#include "../component/VisualModel.h"
#include "../typedef.h"

#include <memory>
#include <snac-renderer/Mesh.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

const std::shared_ptr<snac::Model> nullModel = std::make_shared<snac::Model>();

void PlayerInvulFrame::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase hitStunAndInvul;
    mPlayer.each([aDelta, this, &hitStunAndInvul](
                     component::PlayerLifeCycle & aPlayer,
                     component::PlayerModel & aPlayerModel) {
        // TODO: (franz): this should be better
        if (aPlayer.mIsAlive && aPlayer.mInvulFrameCounter > 0.f)
        {
        
            aPlayer.mInvulFrameCounter -= aDelta;

            if (aPlayer.mHitStun > 0.f)
            {
                aPlayer.mHitStun -= aDelta;
            }

            // TODO: (franz) since there is no alpha
            // we remove the model from the player model
            // but we also need to remove the animation from the player model
            // entity since there is no animation on the null model
            Entity model = *aPlayerModel.mModel.get(hitStunAndInvul);

            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 0
                && !model.has<component::VisualModel>())
            {
                model.add(component::VisualModel{
                    .mModel = mGameContext->mResources.getModel(
                        "models/donut/donut.gltf",
                        "effects/MeshTextures.sefx")});
            }
            if (static_cast<int>(aPlayer.mInvulFrameCounter * 10.f) % 4 == 2
                && model.has<component::VisualModel>())
            {
                model.remove<component::VisualModel>();
            }
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
