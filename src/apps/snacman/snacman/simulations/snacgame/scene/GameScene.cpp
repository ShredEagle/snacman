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
#include "../component/VisualMesh.h"
#include "../Entities.h"
#include "../EntityWrap.h"  // for Entit...
#include "../GameContext.h" // for GameC...
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../InputConstants.h" // for gJoin
#include "../scene/Scene.h"    // for Trans...
#include "../SceneGraph.h"
#include "../system/DeterminePlayerAction.h"
#include "../system/EatPill.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelCreator.h"
#include "../system/MovementIntegration.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"
#include "../system/PortalManagement.h"
#include "../system/RoundMonitor.h"
#include "../system/SceneGraphResolver.h"
#include "../typedef.h"

#include <algorithm>
#include <array>   // for array
#include <cstddef> // for size_t
#include <map>     // for opera...
#include <snacman/Input.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <snacman/Resources.h>
#include <tuple>  // for get
#include <vector> // for vector

namespace ad {
namespace snacgame {
namespace scene {

const char * const gMarkovRoot{"markov/"};

namespace {

EntHandle createLevel(ent::EntityManager & aWorld, GameContext & aContext)
{
    EntHandle level = aWorld.addEntity();
    {
        ent::Phase createLevel;
        auto markovRoot = aContext.mResources.find(gMarkovRoot);
        ent::Entity levelEntity = *level.get(createLevel);
        levelEntity.add(component::LevelData(
            aWorld, markovRoot.value(), "snaclvl.xml", {15, 15, 1}, 123123));
        levelEntity.add(component::LevelToCreate{});
        levelEntity.add(component::SceneNode{});
        levelEntity.add(component::Geometry{.mPosition = {-7.f, -7.f, 0.f}});
        levelEntity.add(component::GlobalPose{});
    }
    return level;
}

}

GameScene::GameScene(const std::string & aName,
                     ent::EntityManager & aWorld,
                     EntityWrap<component::MappingContext> & aContext,
                     EntHandle aSceneRoot,
                     GameContext & aGameContext) :
    Scene(aName, aWorld, aContext, aSceneRoot),
    mLevel{createLevel(mWorld, aGameContext)},
    mTiles{mWorld},
    mSlots{mWorld},
    mPlayers{mWorld}
{}

void GameScene::setup(GameContext & aContext,
                      const Transition & aTransition,
                      RawInput & aInput)
{
    {
        ent::Phase init;
        mSystems.get(init)->add(system::SceneGraphResolver{mWorld, mSceneRoot});
        mSystems.get(init)->add(system::PlayerSpawner{mWorld, mLevel});
        mSystems.get(init)->add(system::RoundMonitor{mWorld});
        mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});
        mSystems.get(init)->add(system::DeterminePlayerAction{mWorld, mLevel});
        mSystems.get(init)->add(system::IntegratePlayerMovement{mWorld});
        mSystems.get(init)->add(system::LevelCreator{&mWorld});
        mSystems.get(init)->add(system::MovementIntegration{mWorld});
        mSystems.get(init)->add(system::EatPill{mWorld});
        mSystems.get(init)->add(system::PortalManagement{mWorld, mLevel});
    }

    // Can't insert mLevel before the createLevel phase is over
    // otherwise mLevel does not have the correct component
    insertEntityInScene(mLevel, mSceneRoot);
}

void GameScene::teardown(GameContext & aContext, RawInput & aInput)
{
    ent::Phase destroy;

    removeEntityFromScene(*mSceneRoot.get(destroy)->get<component::SceneNode>().aFirstChild);

    mSystems.get(destroy)->erase();
    mSystems = mWorld.addEntity();

    mLevel.get(destroy)->erase();
    mLevel = createLevel(mWorld, aContext);

    mTiles.each([&destroy](EntHandle aHandle, const component::LevelEntity &) {
        aHandle.get(destroy)->erase();
    });

    mPlayers.each([&destroy](EntHandle aHandle, component::PlayerSlot & aSlot,
                             component::Controller & aController) {

        removePlayerFromGame(destroy, aHandle);
    });
}

std::optional<Transition>
GameScene::update(GameContext & aContext, float aDelta, RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    ent::Phase update;

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

    ent::Phase bindPlayerPhase;

    // This works because gKeyboardControllerIndex is -1
    // So this is a bit janky but it unifies the player join code
    for (std::size_t controlIndex = gKeyboardControllerIndex;
         controlIndex < aInput.mGamepads.size(); ++controlIndex)
    {
        if (std::find(boundControllers.begin(), boundControllers.end(),
                      gKeyboardControllerIndex)
            == boundControllers.end())
        {
            int command = gNoCommand;
            const bool controllerIsKeyboard =
                (int)controlIndex == gKeyboardControllerIndex;

            if (controllerIsKeyboard)
            {
                convertKeyboardInput("unbound", aInput.mKeyboard,
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
                OptEntHandle player = findSlotAndBind(
                    aContext, bindPlayerPhase, mSlots,
                    controllerIsKeyboard ? ControllerType::Keyboard
                                         : ControllerType::Gamepad,
                    static_cast<int>(controlIndex));
                if (player)
                {
                    insertEntityInScene(*player, mLevel);
                }
            }
        }
    }

    if (quit)
    {
        return Transition{.mTransitionName = "back"};
    }

    mSystems.get(update)->get<system::LevelCreator>().update(aContext);
    mSystems.get(update)->get<system::RoundMonitor>().update();
    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::PortalManagement>().update();
    mSystems.get(update)->get<system::DeterminePlayerAction>().update();
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);
    mSystems.get(update)->get<system::MovementIntegration>().update(aDelta);

    mSystems.get(update)->get<system::SceneGraphResolver>().update();
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);
    mSystems.get(update)->get<system::EatPill>().update();

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
