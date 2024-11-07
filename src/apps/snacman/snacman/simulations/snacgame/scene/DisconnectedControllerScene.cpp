#include "DisconnectedControllerScene.h"

#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/scene/GameScene.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/MenuManager.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/MenuItem.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"
#include "../component/SceneNode.h"
#include "../component/Tags.h"
#include "../component/Text.h"
#include "../system/SceneStack.h"
#include "../GameContext.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <entity/EntityManager.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace scene {

int safePrint(char * buffer, const char * format, ...)
{
    va_list arg;
    va_start (arg, format);
#ifdef _MSC_VER
        int ret = sprintf_s(buffer, INT_MAX, format, arg);
#else
        int ret = sprintf(buffer, format, arg);
#endif
    va_end(arg);
    return ret;
}

DisconnectedControllerScene::DisconnectedControllerScene(
    GameContext & aGameContext,
    ent::Wrap<component::MappingContext> & aContext) :
    Scene(gDisconnectedControllerSceneName, aGameContext, aContext),
    mItems{mGameContext.mWorld},
    mSlots{mGameContext.mWorld}
{
    Phase init;
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");

    auto startHandle = createMenuItem(
        mGameContext, init, "Kick him", font,
        math::Position<2, float>{-0.25f, 0.3f},
        {
            {gGoDown, "Resume"},
        },
        scene::Transition{.mTransitionName = sKickTransitionName}, true,
        {1.5f, 1.5f});
    auto resumeHandle = createMenuItem(
        mGameContext, init, "Resume", font,
        math::Position<2, float>{-0.25f, 0.0f},
        {
            {gGoUp, "Kick him"},
            {gGoDown, "Quit"},
        },
        scene::Transition{.mTransitionName = GameScene::sFromPauseTransition},
        false, {1.5f, 1.5f});
    auto quitHandle = createMenuItem(
        mGameContext, init, "Quit", font,
        math::Position<2, float>{-0.25f, -0.3f},
        {
            {gGoUp, "Resume"},
        },
        scene::Transition{.mTransitionName = gQuitTransitionName}, false,
        {1.5f, 1.5f});
    mOwnedEntities.push_back(startHandle);
    mOwnedEntities.push_back(resumeHandle);
    mOwnedEntities.push_back(quitHandle);
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::MenuManager{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});
}

constexpr std::array<const char *, 2> gControllersTextStart{ 
    "Controller ",
    "Controllers "
};

constexpr std::array<const char *, 2> gControllersTextEnd{ 
    "is disconnected",
    "are disconnected"
};

void DisconnectedControllerScene::onEnter(Transition aTransition)
{
    char sceneTitle[128];
    auto & info = std::get<DisconnectedControllerInfo>(aTransition.mSceneInfo);
    std::array<int, 4> & disconnectedControllers = info.mDisconnectedControllerId;
    int disconnectedCount = info.mDisconnectedCount;

    safePrint(sceneTitle, gControllersTextStart.at((bool)disconnectedCount));

    unsigned int stringCursor = (unsigned int)strlen(sceneTitle);

    for (int i = 0; i < disconnectedCount; i++)
    {
        safePrint(sceneTitle + stringCursor, "%d and ", disconnectedControllers.at(i));
        stringCursor = (unsigned int)strlen(sceneTitle);
    }

    stringCursor -= (unsigned int)strlen("and ");

    safePrint(sceneTitle + stringCursor, gControllersTextEnd.at((bool)disconnectedCount));

    Phase onEnterPhase;
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");
    auto titleHandle =
        makeText(mGameContext, onEnterPhase, sceneTitle, font,
                 math::hdr::gYellow<float>, {-0.75f, 0.6f}, {1.5f, 1.5f});
    mOwnedEntities.push_back(titleHandle);
}

void DisconnectedControllerScene::onExit(Transition aTransition)
{
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mOwnedEntities.clear();
    mSystems.get(destroy)->erase();
}

void DisconnectedControllerScene::update(const snac::Time & aTime,
                                         RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);
    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "menu", "menu");

    auto menuResult = mSystems.get()->get<system::MenuManager>().manageMenu(
            controllerCommands);

    auto menuItem = std::get<0>(menuResult);
    auto accumulatedCommand = std::get<1>(menuResult);
        
    if (accumulatedCommand & gQuitCommand)
    {
        mGameContext.mSceneStack->popScene(
            {.mTransitionName = gQuitTransitionName});
        return;
    }

    if (menuItem.mTransition.mTransitionName == GameScene::sFromPauseTransition
        && accumulatedCommand & gSelectItem)
    {
        mGameContext.mSceneStack->popScene();
        return;
    }
    if (menuItem.mTransition.mTransitionName == GameScene::sFromPauseTransition
        && accumulatedCommand & gSelectItem)
    {
        mGameContext.mSceneStack->popScene();
        return;
    }
    if (menuItem.mTransition.mTransitionName == gQuitTransitionName
        && accumulatedCommand & gSelectItem)
    {
        mGameContext.mSceneStack->popScene(menuItem.mTransition);
        return;
    }

    bool stillDisconnectedController = false;
    Phase destroyDisconnectedPlayer;
    mSlots.each(
        [&aInput, &stillDisconnectedController, &menuItem,
         &destroyDisconnectedPlayer, &accumulatedCommand](
            EntHandle aSlotHandle, component::PlayerSlot & aSlot,
            component::PlayerGameData & aGameData)
        {
        const component::Controller & controller =
            snac::getComponent<component::Controller>(aSlot.mPlayer);

        // A keyboard can't be disconnected for the moment (not logically)
        if (controller.mType == ControllerType::Keyboard)
        {
            return;
        }

        GamepadState padState = aInput.mGamepads.at(controller.mControllerId);
        bool controllerIsDisconnected = !padState.mConnected;

        if (menuItem.mTransition.mTransitionName == sKickTransitionName
            && accumulatedCommand & gSelectItem)
        {
            EntHandle aPlayerModelHandle = aSlot.mPlayer;
            EntHandle aHudHandle = aGameData.mHud;
            eraseEntityRecursive(aHudHandle, destroyDisconnectedPlayer);
            eraseEntityRecursive(aPlayerModelHandle, destroyDisconnectedPlayer);
            eraseEntityRecursive(aSlotHandle, destroyDisconnectedPlayer);
        }
        else
        {
            stillDisconnectedController |= controllerIsDisconnected;
        }
    });

    if (!stillDisconnectedController)
    {
        mGameContext.mSceneStack->popScene(
            {.mTransitionName = GameScene::sFromPauseTransition});
        return;
    }

    mSystems.get()->get<system::SceneGraphResolver>().update();
    return;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
