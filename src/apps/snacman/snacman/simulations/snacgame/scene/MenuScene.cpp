#include "MenuScene.h"

#include "../../../Input.h"
#include "../component/Controller.h"
#include "../component/PlayerSlot.h"
#include "../Entities.h"
#include "../InputCommandConverter.h"
#include "../SnacGame.h"

#include <optional>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(GameContext & aContext, const Transition & Transition, RawInput & aInput)
{
    auto font = aContext.mRenderThread.loadFont("fonts/Comfortaa-Regular.ttf", 120, aContext.mResource).get();
    ent::Phase init;
    auto start = createMenuItem(
        aContext, init, "start",
        font,
        math::hdr::gYellow<float>,
        math::Position<2, float>{0.f, 0.f});
    auto quit = createMenuItem(
        aContext, init, "quit",
        font,
        math::hdr::gYellow<float>,
        math::Position<2, float>{0.f, -0.2f});
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

    ent::Phase bindPlayerPhase;

    bool boundPlayer = false;

    if (keyboardCommand & gSelectItem)
    {
        boundPlayer =
            findSlotAndBind(aContext, bindPlayerPhase, mSlots, ControllerType::Keyboard,
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
            boundPlayer |= findSlotAndBind(aContext, bindPlayerPhase, mSlots,
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
