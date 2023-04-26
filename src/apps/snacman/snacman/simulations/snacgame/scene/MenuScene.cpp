#include "MenuScene.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include "../Entities.h"
#include "../InputCommandConverter.h"
#include "../SnacGame.h"

#include "../component/Controller.h"
#include "../component/Context.h"
#include "../component/PlayerSlot.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/SceneNode.h"



#include <snacman/Input.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>

#include <optional>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(const Transition & Transition,
                      RawInput & aInput)
{
    auto font = mGameContext.mResources.getFont("fonts/FredokaOne-Regular.ttf");

    ent::Phase init;
    auto startHandle = createMenuItem(
        mGameContext, init, "Start", font, math::Position<2, float>{-0.55f, 0.0f},
        {
            {gGoDown, "quit"},
        },
        scene::Transition{.mTransitionName = "start"}, true, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "quit", font, math::Position<2, float>{-0.55f, -0.3f},
        {
            {gGoUp, "Start"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {1.5f, 1.5f});

    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mSceneRoot});

    ent::Handle<ent::Entity> background = createStageDecor(mGameContext);
    mOwnedEntities.push_back(background);
    insertEntityInScene(background, mSceneRoot);
}

void MenuScene::teardown(RawInput & aInput)
{
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mOwnedEntities.clear();
    mSystems.get(destroy)->erase();
    mSystems = mGameContext.mWorld.addEntity();
}

std::optional<Transition>
MenuScene::update(float aDelta, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);
    ControllerCommand keyboardCommand{
        .mCommand = convertKeyboardInput("menu", aInput.mKeyboard,
                                         mContext->mKeyboardMapping),
        .mId = gKeyboardControllerIndex,
        .mControllerType = ControllerType::Keyboard,
    };

    int accumulatedCommand = keyboardCommand.mCommand;

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

    mItems.each([this, &menuTransition, accumulatedCommand,
                 keyboardCommand, &controllerCommandList,
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
                    findSlotAndBind(mGameContext, mSlots,
                                    keyboardCommand.mControllerType,
                                    keyboardCommand.mId);
                    menuTransition = aItem.mTransition;
                }
                for (auto command : controllerCommandList)
                {
                    if (command.mCommand & gSelectItem)
                    {
                        findSlotAndBind(mGameContext, mSlots,
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

    ent::Phase update;
    mSystems.get(update)->get<system::SceneGraphResolver>().update();

    return menuTransition;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
