#include "PauseScene.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/scene/GameScene.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/MenuManager.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include "../component/MenuItem.h"
#include "../component/Tags.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/PathToOnGrid.h"
#include "../component/Controller.h"
#include "../component/Text.h"
#include "../component/GlobalPose.h"
#include "../component/Geometry.h"
#include "../component/SceneNode.h"
#include "../system/SceneStack.h"
#include "../GameContext.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include <entity/EntityManager.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace scene {

PauseScene::PauseScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gPauseSceneName, aGameContext, aContext), mItems{mGameContext.mWorld}
{
    Phase init;
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");

    auto startHandle = createMenuItem(
        mGameContext, init, "Resume", font,
        math::Position<2, float>{-0.1f, 0.0f},
        {
            {gGoDown, "Quit"},
        },
        scene::Transition{.mTransitionType =
                              TransType::GameFromPause},
        true, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "Quit", font,
        math::Position<2, float>{-0.1f, -0.3f},
        {
            {gGoUp, "Resume"},
        },
        scene::Transition{.mTransitionType = TransType::QuitToMenu}, false,
        {1.5f, 1.5f});
    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
}

void PauseScene::onEnter(Transition aTransition) {
    // There is 1 transition possible
    // From the game
    mSystems = mGameContext.mWorld.addEntity("System for Pause");
    ent::Phase onEnter;
    mSystems.get(onEnter)->add(
        system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::MenuManager{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});
}

void PauseScene::onExit(Transition aTransition)
{
    // There is 2 transition possible
    // To the game
    // To the main menu
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mOwnedEntities.clear();
    mSystems.get(destroy)->erase();
}

void PauseScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);
    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "menu", "menu");

    auto [menuItem, accumulatedCommand] =
        mSystems.get()->get<system::MenuManager>().manageMenu(
            controllerCommands);

    if (accumulatedCommand & gQuitCommand)
    {
        mGameContext.mSceneStack->popScene({.mTransitionType = TransType::QuitToMenu});
        return;
    }

    if (menuItem.mTransition.mTransitionType
             == TransType::GameFromPause && accumulatedCommand & gSelectItem)
    {

        mGameContext.mSceneStack->popScene();
        return;
    }
    if (menuItem.mTransition.mTransitionType
             == TransType::QuitToMenu && accumulatedCommand & gSelectItem)
    {

        mGameContext.mSceneStack->popScene(menuItem.mTransition);
        return;
    }
    mSystems.get()->get<system::SceneGraphResolver>().update();
    return;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
