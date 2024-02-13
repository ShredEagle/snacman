#include "PlayerInvulFrame.h"

#include "../component/PlayerRoundData.h"
#include "../component/RigAnimation.h"
#include "../component/VisualModel.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include "../typedef.h"
#include "../GameContext.h"

#include <snacman/Profiling.h>

#include <snac-renderer-V1/Mesh.h>

#include <entity/EntityManager.h>

#include <memory>

namespace ad {
namespace snacgame {
namespace system {

const std::shared_ptr<snac::Model> nullModel = std::make_shared<snac::Model>();

PlayerInvulFrame::PlayerInvulFrame(GameContext & aGameContext) :
    mGameContext{&aGameContext}, mPlayer{mGameContext->mWorld}
{}

void PlayerInvulFrame::update(float aDelta)
{
    TIME_RECURRING_CLASSFUNC(Main);

    // TODO: (franz) this is needed because we can't set the model 
    // of the player to a null model because of the animation component
    ent::Phase hitStunAndInvul;
    mPlayer.each([aDelta, this, &hitStunAndInvul](
                     component::PlayerRoundData & aRoundData) {
        // TODO: (franz): this should be better
        if (aRoundData.mInvulFrameCounter > 0.f)
        {
        
            aRoundData.mInvulFrameCounter -= aDelta;

            // TODO: (franz) since there is no alpha
            // we remove the model from the player model
            // but we also need to remove the animation from the player model
            // entity since there is no animation on the null model
            Entity model = *aRoundData.mModel.get(hitStunAndInvul);

            if (static_cast<int>(aRoundData.mInvulFrameCounter * 10.f) % 4 == 0
                && !model.has<component::VisualModel>())
            {
                model.add(component::VisualModel{
                    .mModel = mGameContext->mResources.getModel(
                        "models/donut/donut.gltf",
                        "effects/MeshTextures.sefx")});
            }
            if (static_cast<int>(aRoundData.mInvulFrameCounter * 10.f) % 4 == 2
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
