#include "MenuScene.h"

#include "snacman/Logging.h"

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

void MenuScene::setup(GameContext & aContext,
                      const Transition & Transition,
                      RawInput & aInput)
{
    auto font =
        aContext.mRenderThread
            .loadFont("fonts/Comfortaa-Regular.ttf", 120, aContext.mResource)
            .get();

    ent::Phase init;
    auto startHandle =
        createMenuItem(aContext, init, "start", font, gColorItemSelected,
                       math::Position<2, float>{0.f, 0.f},
                       {
                           {gGoDown, "quit"},
                       },
                       true);
    auto quitHandle =
        createMenuItem(aContext, init, "quit", font, gColorItemUnselected,
                       math::Position<2, float>{0.f, -0.2f},
                       {
                           {gGoUp, "start"},
                       });

    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
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

std::optional<Transition>
MenuScene::update(GameContext & aContext, float aDelta, RawInput & aInput)
{
    int keyboardCommand = convertKeyboardInput("menu", aInput.mKeyboard,
                                               mContext->mKeyboardMapping);
    int accumulatedCommand = keyboardCommand;

    if (keyboardCommand & gQuitCommand)
    {
        return Transition{.mTransitionName = gQuitTransitionName};
    }

    ent::Phase bindPlayerPhase;

    bool boundPlayer = false;

    if (keyboardCommand & gSelectItem)
    {
        boundPlayer =
            findSlotAndBind(aContext, bindPlayerPhase, mSlots,
                            ControllerType::Keyboard, gKeyboardControllerIndex);
    }

    for (std::size_t index = 0; index < aInput.mGamepads.size(); ++index)
    {
        GamepadState & rawGamepad = aInput.mGamepads.at(index);
        int gamepadCommand =
            convertGamepadInput("menu", rawGamepad, mContext->mGamepadMapping);
        accumulatedCommand |= gamepadCommand;

        if (gamepadCommand & gQuitCommand)
        {
            return Transition{.mTransitionName = gQuitTransitionName};
        }

        if (gamepadCommand & gSelectItem)
        {
            boundPlayer |= findSlotAndBind(aContext, bindPlayerPhase, mSlots,
                                           ControllerType::Gamepad,
                                           static_cast<int>(index));
        }
    }

    if (boundPlayer)
    {
        return Transition{.mTransitionName = "start"};
    }

    std::string newItem;
    int filteredForMenuCommand =
        accumulatedCommand & (gGoUp | gGoDown | gGoLeft | gGoRight);
    mItems.each([filteredForMenuCommand, &newItem](component::MenuItem & aItem,
                                                   component::Text & aText) {
        if (aItem.mSelected
            && aItem.mNeighbors.contains(filteredForMenuCommand))
        {
            newItem = aItem.mNeighbors.at(filteredForMenuCommand);
            aItem.mSelected = false;
            aText.mColor = gColorItemUnselected;
        }
    });

    mItems.each(
        [&newItem](component::MenuItem & aItem, component::Text & aText) {
            if (aItem.mName == newItem)
            {
                aItem.mSelected = true;
                aText.mColor = gColorItemSelected;
            }
        });

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
