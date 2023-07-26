#include "PauseScene.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/scene/GameScene.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/MenuManager.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include "../component/MenuItem.h"
#include "../component/Controller.h"
#include "../component/Text.h"
#include "../component/GlobalPose.h"
#include "../component/Geometry.h"
#include "../component/SceneNode.h"
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
        scene::Transition{.mTransitionName =
                              GameScene::sFromPauseTransition},
        true, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "Quit", font,
        math::Position<2, float>{-0.1f, -0.3f},
        {
            {gGoUp, "Resume"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {1.5f, 1.5f});
    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(quitHandle);
    mSystems.get(init)->add(
        system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::MenuManager{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});
}

void PauseScene::onEnter(Transition aTransition) {}

void PauseScene::onExit(Transition aTransition)
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

void PauseScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);
    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "menu", "menu");

    auto [menuItem, accumulatedCommand] =
        mSystems.get()->get<system::MenuManager>().manageMenu(
            controllerCommands);
    if (menuItem.mTransition.mTransitionName
             == GameScene::sFromPauseTransition && accumulatedCommand & gSelectItem)
    {

        mGameContext.mSceneStack->popScene();
        return;
    }
    if (menuItem.mTransition.mTransitionName
             == gQuitTransitionName && accumulatedCommand & gSelectItem)
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
