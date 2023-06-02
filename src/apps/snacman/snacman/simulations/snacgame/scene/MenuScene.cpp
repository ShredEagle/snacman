#include "MenuScene.h"

#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/PlayerSlot.h"
#include "../component/SceneNode.h"
#include "../component/MenuItem.h"
#include "../component/Text.h"

#include "../system/SceneGraphResolver.h"

#include "../SceneGraph.h"
#include "../Entities.h"
#include "../InputCommandConverter.h"
#include "../SnacGame.h"

#include <cstring>
#include <optional>
#include <snacman/Input.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace scene {

MenuScene::MenuScene(std::string aName,
        GameContext & aGameContext,
          EntityWrap<component::MappingContext> & aContext,
          ent::Handle<ent::Entity> aSceneRoot) :
    Scene(aName, aGameContext, aContext, aSceneRoot), mItems{mGameContext.mWorld}
{}

void MenuScene::setup(const Transition & Transition, RawInput & aInput)
{
    auto font = mGameContext.mResources.getFont("fonts/FredokaOne-Regular.ttf");

    ent::Phase init;
    auto startHandle = createMenuItem(
        mGameContext, init, "Start", font,
        math::Position<2, float>{-0.55f, 0.0f},
        {
            {gGoDown, "quit"},
        },
        scene::Transition{.mTransitionName = "start"}, true, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "quit", font,
        math::Position<2, float>{-0.55f, -0.3f},
        {
            {gGoUp, "Start"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {1.5f, 1.5f});

    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
    mSystems.get(init)->add(
        system::SceneGraphResolver{mGameContext, mSceneRoot});

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

std::optional<Transition> MenuScene::update(const snac::Time & aTime,
                                            RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);

    std::vector<ControllerCommand> controllerCommandList;

    ControllerCommand keyboardCommand{
        .mId = gKeyboardControllerIndex,
        .mControllerType = ControllerType::Keyboard,
        .mInput = convertKeyboardInput("menu", aInput.mKeyboard,
                                       mContext->mKeyboardMapping),
    };

    int accumulatedCommand = keyboardCommand.mInput.mCommand;

    controllerCommandList.push_back(keyboardCommand);

    for (std::size_t index = 0; index < aInput.mGamepads.size(); ++index)
    {
        GamepadState & rawGamepad = aInput.mGamepads.at(index);
        ControllerCommand gamepadCommand{
            .mId = static_cast<int>(index),
            .mControllerType = ControllerType::Gamepad,
            .mInput = convertGamepadInput("menu", rawGamepad,
                                          mContext->mGamepadMapping),
        };
        accumulatedCommand |= gamepadCommand.mInput.mCommand;
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

    mItems.each(
        [this, &menuTransition, accumulatedCommand, &controllerCommandList,
         filteredForMenuCommand, &newItem]
        (component::MenuItem & aItem, component::Text & aText)
    {
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
                for (auto command : controllerCommandList)
                {
                    if (command.mInput.mCommand & gSelectItem)
                    {
                        addPlayer(mGameContext,
                                        command.mId, command.mControllerType);
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
    mItems.each(
        [&newItem, &newItemFound](component::MenuItem & aItem,
                                  component::Text & aText)
        {
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

    mSystems.get()->get<system::SceneGraphResolver>().update();

    return menuTransition;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
