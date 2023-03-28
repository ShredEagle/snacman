#include "MenuScene.h"

#include "snacman/Input.h"
#include "snacman/Logging.h"
#include "snacman/simulations/snacgame/component/Controller.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"

#include "../../../Input.h"
#include "../component/Controller.h"
#include "../component/Context.h"
#include "../component/PlayerSlot.h"
#include "../Entities.h"
#include "../InputCommandConverter.h"
#include "../SnacGame.h"

#include <optional>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(GameContext & aContext,
                      const Transition & Transition,
                      RawInput & aInput)
{
    auto font = aContext.mResources.getFont("fonts/FredokaOne-Regular.ttf");

    ent::Phase init;
    auto startHandle = createMenuItem(
        aContext, init, "Start", font, math::Position<2, float>{-0.55f, 0.0f},
        {
            {gGoDown, "quit"},
        },
        scene::Transition{.mTransitionName = "start"}, true, {3.f, 3.f});
    auto quitHandle = createMenuItem(
        aContext, init, "quit", font, math::Position<2, float>{-0.55f, -0.3f},
        {
            {gGoUp, "Start"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {3.f, 3.f});

    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
}

void MenuScene::teardown(GameContext & aContext, RawInput & aInput)
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
    TIME_RECURRING_CLASSFUNC(Main);
    ControllerCommand keyboardCommand{
        .mCommand = convertKeyboardInput("menu", aInput.mKeyboard,
                                         mContext->mKeyboardMapping),
        .mId = gKeyboardControllerIndex,
        .mControllerType = ControllerType::Keyboard,
    };

    int accumulatedCommand = keyboardCommand.mCommand;

    ent::Phase bindPlayerPhase;

    std::vector<ControllerCommand> controllerCommandList;

    for (std::size_t index = 0; index < aInput.mGamepads.size(); ++index)
    {
        GamepadState & rawGamepad = aInput.mGamepads.at(index);
        ControllerCommand gamepadCommand{
            .mCommand = convertGamepadInput("menu", rawGamepad,
                                            mContext->mGamepadMapping),
            .mId = static_cast<int>(index),
            .mControllerType = ControllerType::Gamepad,
        };
        accumulatedCommand |= gamepadCommand.mCommand;
        controllerCommandList.push_back(gamepadCommand);
    }

    if (accumulatedCommand & gQuitCommand)
    {
        return Transition{.mTransitionName = gQuitTransitionName};
    }

    std::string newItem;

    int filteredForMenuCommand =
        accumulatedCommand & (gGoUp | gGoDown | gGoLeft | gGoRight);

    std::optional<Transition> menuTransition = std::nullopt;

    mItems.each([this, &menuTransition, accumulatedCommand, &bindPlayerPhase,
                 &aContext, keyboardCommand, &controllerCommandList,
                 filteredForMenuCommand, &newItem](component::MenuItem & aItem,
                                                   component::Text & aText) {
        if (aItem.mSelected)
        {
            if (aItem.mNeighbors.contains(filteredForMenuCommand))
            {
                newItem = aItem.mNeighbors.at(filteredForMenuCommand);
                aItem.mSelected = false;
                aText.mColor = gColorItemUnselected;
            }
            else if (aItem.mTransition.mTransitionName == "start")
            {
                if (keyboardCommand.mCommand & gSelectItem)
                {
                    findSlotAndBind(aContext, bindPlayerPhase, mSlots,
                                    keyboardCommand.mControllerType,
                                    keyboardCommand.mId);
                    menuTransition = aItem.mTransition;
                }
                for (auto command : controllerCommandList)
                {
                    if (command.mCommand & gSelectItem)
                    {
                        findSlotAndBind(aContext, bindPlayerPhase, mSlots,
                                        command.mControllerType, command.mId);
                        menuTransition = aItem.mTransition;
                    }
                }
            }
            if (aItem.mTransition.mTransitionName == "quit")
            {
                if (accumulatedCommand & gSelectItem)
                {
                    menuTransition = aItem.mTransition;
                }
            }
        }
    });

    bool newItemFound = false;
    mItems.each([&newItem, &newItemFound](component::MenuItem & aItem,
                                          component::Text & aText) {
        if (aItem.mName == newItem)
        {
            newItemFound = true;
            aItem.mSelected = true;
            aText.mColor = gColorItemSelected;
        }
    });

    if (!newItemFound && newItem != "")
    {
        SELOG(error)
        ("Could not find item {} in menu (Check the case)", newItem);
    }

    return menuTransition;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
