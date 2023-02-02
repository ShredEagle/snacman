#include "MenuScene.h"

#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/Controller.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"

#include "../Entities.h"

#include <optional>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(GameContext & aContext, const Transition & Transition, RawInput & aInput)
{
    ent::Phase init;
    auto start = createMenuItem(aContext, init, math::Position<2, int>{0, 0});
    auto quit = createMenuItem(aContext, init, math::Position<2, int>{2, 0});
    mOwnedEntities.push_back(start);
    mOwnedEntities.push_back(quit);
}

void MenuScene::teardown(RawInput & aInput)
{
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mOwnedEntities.clear();
}

std::optional<Transition> MenuScene::update(GameContext & aContext,
                                            float aDelta,
                                            RawInput & aInput)
{
    int keyboardCommand = convertKeyboardInput("menu", aInput.mKeyboard,
                                               mContext->mKeyboardMapping);

    if (keyboardCommand & gQuitCommand)
    {
        return Transition{.mTransitionName = gQuitTransitionName};
    }

    if (keyboardCommand & gSelectItem)
    {
        ent::Phase bindPlayerPhase;

        mSlots.each([&](ent::Handle<ent::Entity> aHandle,
                        component::PlayerSlot & aSlot) {
            if (!aSlot.mFilled && !aInput.mKeyboard.mBound)
            {
                fillSlotWithPlayer(aContext,
                                   bindPlayerPhase,
                                   component::ControllerType::Keyboard, aHandle,
                                   0);
                aInput.mKeyboard.mBound = true;
            }
        });

        return Transition{.mTransitionName = "start"};
    }

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
