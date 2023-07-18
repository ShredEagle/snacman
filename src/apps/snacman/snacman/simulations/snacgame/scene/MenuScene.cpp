#include "MenuScene.h"

#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/MenuItem.h"
#include "../component/PlayerSlot.h"
#include "../component/SceneNode.h"
#include "../component/Text.h"
#include "../Entities.h"
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../scene/DataScene.h"
#include "../scene/GameScene.h"
#include "../scene/JoinGameScene.h"
#include "../SceneGraph.h"
#include "../SnacGame.h"
#include "../system/SceneGraphResolver.h"
#include "../typedef.h"

#include <entity/EntityManager.h>
#include <optional>
#include <snacman/EntityUtilities.h>
#include <snacman/Input.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <snacman/Timing.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

MenuScene::MenuScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gMenuSceneName, aGameContext, aContext), mItems{mGameContext.mWorld}
{
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");

    ent::Phase init;
    auto startHandle = createMenuItem(
        mGameContext, init, "Start", font,
        math::Position<2, float>{-0.55f, 0.0f},
        {
            {gGoDown, "quit"},
        },
        scene::Transition{.mTransitionName =
                              JoinGameScene::sFromMenuTransition},
        true, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "Quit", font,
        math::Position<2, float>{-0.55f, -0.3f},
        {
            {gGoUp, "Start"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {1.5f, 1.5f});

    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
    mSystems.get(init)->add(
        system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot});

    ent::Handle<ent::Entity> background = createStageDecor(mGameContext);
    mOwnedEntities.push_back(background);
    insertEntityInScene(background, mGameContext.mSceneRoot);

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    snac::Orbital & camOrbital = snac::getComponent<snac::Orbital>(camera);
    camOrbital.mSpherical = gInitialCameraSpherical;
}

void MenuScene::onEnter(Transition aTransition) {}

void MenuScene::onExit(Transition aTransition)
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

void MenuScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);

    std::vector<ControllerCommand> controllerCommandList;

    ControllerCommand keyboardCommand{
        .mId = gKeyboardControllerIndex,
        .mControllerType = ControllerType::Keyboard,
        .mInput = convertKeyboardInput("menu", aInput.mKeyboard,
                                       mMappingContext->mKeyboardMapping),
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
                                          mMappingContext->mGamepadMapping),
        };
        accumulatedCommand |= gamepadCommand.mInput.mCommand;
        controllerCommandList.push_back(gamepadCommand);
    }

    bool quit = false;
    if (accumulatedCommand & gQuitCommand)
    {
        quit = true;
    }

    std::string newItem;

    int filteredForMenuCommand =
        accumulatedCommand & (gGoUp | gGoDown | gGoLeft | gGoRight);

    EntHandle selectedItemHandle = snac::getFirstHandle(
        mItems, [](component::MenuItem & aItem) { return aItem.mSelected; });
    component::MenuItem & menuItem =
        snac::getComponent<component::MenuItem>(selectedItemHandle);
    component::Text & text =
        snac::getComponent<component::Text>(selectedItemHandle);

    if (menuItem.mNeighbors.contains(filteredForMenuCommand))
    {
        newItem = menuItem.mNeighbors.at(filteredForMenuCommand);
        menuItem.mSelected = false;
        text.mColor = gColorItemUnselected;
    }
    else if (menuItem.mTransition.mTransitionName
             == JoinGameScene::sFromMenuTransition)
    {
        for (auto command : controllerCommandList)
        {
            if (command.mInput.mCommand & gSelectItem)
            {
                Transition menuTransition = menuItem.mTransition;
                menuTransition.mSceneInfo = {JoinGameSceneInfo{command.mId}};
                // Menu pops itself from the stack
                mGameContext.mSceneStack->popScene();
                mGameContext.mSceneStack->pushScene(
                    std::make_shared<JoinGameScene>(mGameContext,
                                                    mMappingContext),
                    menuTransition);
                return;
            }
        }
    }
    else if (menuItem.mTransition.mTransitionName == "quit")
    {
        quit = true;
    }

    if (quit)
    {
        // Menu pops itself from the stack
        mGameContext.mSceneStack->popScene(
            Transition{.mTransitionName = StageDecorScene::sQuitTransition});
        return;
    }

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

    return;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
