// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj

#include "GameScene.h"

#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/LevelData.h"
#include "../component/LevelTags.h" // for Level...
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerMoveState.h"
#include "../component/PlayerSlot.h" // for Playe...
#include "../component/PoseScreenSpace.h"
#include "../component/Speed.h"
#include "../component/Text.h"
#include "../component/VisualModel.h"

#include "../system/AllowMovement.h"
#include "../system/ConsolidateGridMovement.h"
#include "../system/Debug_BoundingBoxes.h"
#include "../system/EatPill.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelCreator.h"
#include "../system/MovementIntegration.h"
#include "../system/Pathfinding.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"
#include "../system/PortalManagement.h"
#include "../system/PowerUpUsage.h"
#include "../system/RoundMonitor.h"
#include "../system/SceneGraphResolver.h"

#include "../Entities.h"
#include "../EntityWrap.h"  // for Entit...
#include "../GameContext.h" // for GameC...
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../InputConstants.h" // for gJoin
#include "../scene/Scene.h"    // for Trans...
#include "../SceneGraph.h"
#include "../typedef.h"

#include <snacman/Input.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <snacman/Resources.h>

#include <algorithm>
#include <array>   // for array
#include <cstddef> // for size_t
#include <map>     // for opera...
#include <tuple>  // for get
#include <vector> // for vector

namespace ad {
namespace snacgame {
namespace scene {

const char * const gMarkovRoot{"markov/"};

namespace {

EntHandle createLevel(GameContext & aContext, const char * aLvlFile)
{
    EntHandle level = aContext.mWorld.addEntity();
    {
        Phase createLevel;
        auto markovRoot = aContext.mResources.find(gMarkovRoot);
        Entity levelEntity = *level.get(createLevel);
        levelEntity.add(component::LevelData(aContext.mWorld,
                                             markovRoot.value(), aLvlFile,
                                             {15, 15, 1}, 123123));
        levelEntity.add(component::SceneNode{});
        levelEntity.add(component::Geometry{.mPosition = {-7.f, -7.f, 0.f}});
        levelEntity.add(component::GlobalPose{});
    }
    aContext.mLevel = level;
    return level;
}

void teardownLevel(GameContext & aGameContext, Phase & aPhase)
{
    aGameContext.mLevel->get(aPhase)->remove<component::SceneNode>();
    aGameContext.mLevel->get(aPhase)->remove<component::LevelCreated>();
    aGameContext.mLevel->get(aPhase)->remove<component::Geometry>();
    aGameContext.mLevel->get(aPhase)->remove<component::GlobalPose>();
}

} // unnamed namespace

GameScene::GameScene(const std::string & aName,
                     GameContext & aGameContext,
                     EntityWrap<component::MappingContext> & aContext,
                     EntHandle aSceneRoot) :
    Scene(aName, aGameContext, aContext, aSceneRoot),
    mTiles{mGameContext.mWorld},
    mSlots{mGameContext.mWorld},
    mPlayers{mGameContext.mWorld},
    mPathfinders{mGameContext.mWorld}
{
    createLevel(mGameContext,
                mPlayers.countMatches() == 4 ? "snaclvl4.xml" : "snaclvl3.xml");
}

void GameScene::teardown(RawInput & aInput)
{
    TIME_SINGLE(Main, "teardown game scene");
    {
        Phase destroy;

        teardownLevel(mGameContext, destroy);

        mSystems.get(destroy)->erase();
        mSystems = mGameContext.mWorld.addEntity();

        mTiles.each(
            [&destroy](EntHandle aHandle, const component::LevelEntity &) {
                aHandle.get(destroy)->erase();
            });

        mPlayers.each([&destroy](EntHandle aHandle,
                                 component::PlayerSlot & aSlot,
                                 component::Controller & aController) {
            removePlayerFromGame(destroy, aHandle);
        });
    }
    {
        // TODO: (franz) remove this at some point
        Phase debugDestroy;
        mPathfinders.each([&debugDestroy](EntHandle aHandle,
                                          const component::PathToOnGrid &) {
            aHandle.get(debugDestroy)->erase();
        });
    }
}

std::optional<Transition> GameScene::update(float aDelta, RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    Phase update;

    bool quit = false;

    std::vector<int> boundControllers;

    mPlayers.each([&](component::PlayerSlot & aPlayerSlot,
                      component::Controller & aController) {
        switch (aController.mType)
        {
        case ControllerType::Keyboard:
            aController.mCommandQuery = convertKeyboardInput(
                "player", aInput.mKeyboard, mContext->mKeyboardMapping);
            break;
        case ControllerType::Gamepad:
            aController.mCommandQuery = convertGamepadInput(
                "player", aInput.mGamepads.at(aController.mControllerId),
                mContext->mGamepadMapping);
            break;
        default:
            break;
        }

        boundControllers.push_back(aController.mControllerId);

        quit |= static_cast<bool>(aController.mCommandQuery & gQuitCommand);
    });

    // This works because gKeyboardControllerIndex is -1
    // So this is a bit janky but it unifies the player join code
    for (int controlIndex = gKeyboardControllerIndex;
         controlIndex < (int) aInput.mGamepads.size(); ++controlIndex)
    {
        if (std::find(boundControllers.begin(), boundControllers.end(),
                      controlIndex)
            == boundControllers.end())
        {
            int command = gNoCommand;
            const bool controllerIsKeyboard =
                (int) controlIndex == gKeyboardControllerIndex;

            if (controllerIsKeyboard)
            {
                command = convertKeyboardInput("unbound", aInput.mKeyboard,
                                               mContext->mKeyboardMapping);
            }
            else
            {
                GamepadState & rawGamepad = aInput.mGamepads.at(controlIndex);
                command = convertGamepadInput("unbound", rawGamepad,
                                              mContext->mGamepadMapping);
            }

            if (command & gJoin)
            {
                findSlotAndBind(mGameContext, mSlots,
                                controllerIsKeyboard ? ControllerType::Keyboard
                                                     : ControllerType::Gamepad,
                                static_cast<int>(controlIndex));
            }
        }
    }

    if (quit)
    {
        return Transition{.mTransitionName = "back"};
    }

    mSystems.get(update)->get<system::LevelCreator>().update();

    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::AllowMovement>().update();
    mSystems.get(update)->get<system::ConsolidateGridMovement>().update(aDelta);
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);
    mSystems.get(update)->get<system::MovementIntegration>().update(aDelta);
    mSystems.get(update)->get<system::Pathfinding>().update();
    mSystems.get(update)->get<system::PortalManagement>().preGraphUpdate();

    mSystems.get(update)->get<system::SceneGraphResolver>().update();
    mSystems.get(update)->get<system::PortalManagement>().postGraphUpdate();
    mSystems.get(update)->get<system::PowerUpUsage>().update(aDelta);
    mSystems.get(update)->get<system::EatPill>().update();

    mSystems.get(update)->get<system::RoundMonitor>().update();
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);

    mSystems.get(update)->get<system::Debug_BoundingBoxes>().update();

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
