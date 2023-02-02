#include "MenuScene.h"

#include "../SnacGame.h"

#include "../../../Input.h"
#include "../component/Controller.h"
#include "../component/PlayerSlot.h"
#include "../Entities.h"
#include "../InputCommandConverter.h"

#include <optional>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(const Transition & Transition, RawInput & aInput)
{
    ent::Phase init;
    auto start = createMenuItem(mWorld, init, math::Position<2, int>{0, 0});
    auto quit = createMenuItem(mWorld, init, math::Position<2, int>{2, 0});
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

std::optional<Transition> MenuScene::update(float aDelta, RawInput & aInput)
{
    int keyboardCommand = convertKeyboardInput("menu", aInput.mKeyboard,
                                               mContext->mKeyboardMapping);

    if (keyboardCommand & gQuitCommand)
    {
        return Transition{.mTransitionName = gQuitTransitionName};
    }

    ent::Phase bindPlayerPhase;

    bool boundPlayer = false;

    if (keyboardCommand & gSelectItem)
    {
        boundPlayer =
            findSlotAndBind(bindPlayerPhase, mSlots, ControllerType::Keyboard,
                            gKeyboardControllerIndex);
    }

    for (std::size_t index = 0; index < aInput.mGamepads.size(); ++index)
    {
        GamepadState & rawGamepad = aInput.mGamepads.at(index);
        int gamepadCommand =
            convertGamepadInput("menu", rawGamepad, mContext->mGamepadMapping);

        if (gamepadCommand & gQuitCommand)
        {
            return Transition{.mTransitionName = gQuitTransitionName};
        }

        if (gamepadCommand & gSelectItem)
        {
            boundPlayer |= findSlotAndBind(bindPlayerPhase, mSlots,
                                           ControllerType::Gamepad, static_cast<int>(index));
        }
    }

    if (boundPlayer)
    {
        return Transition{.mTransitionName = "start"};
    }

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
