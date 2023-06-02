// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit:
// compile with /bigobj

#include "GameScene.h"

#include "snacman/EntityUtilities.h"

#include "../component/AllowedMovement.h"
#include "../component/Collision.h"
#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Explosion.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/LevelData.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h" // for Playe...
#include "../component/Portal.h"
#include "../component/PoseScreenSpace.h"
#include "../component/RigAnimation.h"
#include "../component/Spawner.h"
#include "../component/Speed.h"
#include "../component/Tags.h" // for Level...
#include "../component/Text.h"
#include "../component/VisualModel.h"
#include "../Entities.h"
#include "../EntityWrap.h"  // for Entit...
#include "../GameContext.h" // for GameC...
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../InputConstants.h" // for gJoin
#include "../scene/Scene.h"    // for Trans...
#include "../SceneGraph.h"
#include "../system/AdvanceAnimations.h"
#include "../system/AllowMovement.h"
#include "../system/AnimationManager.h"
#include "../system/ConsolidateGridMovement.h"
#include "../system/Debug_BoundingBoxes.h"
#include "../system/EatPill.h"
#include "../system/Explosion.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelManager.h"
#include "../system/MovementIntegration.h"
#include "../system/Pathfinding.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"
#include "../system/PortalManagement.h"
#include "../system/PowerUpUsage.h"
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
#include <string>
#include <tuple>  // for get
#include <vector> // for vector

namespace ad {
namespace snacgame {
namespace scene {

const char * const gMarkovRoot{"markov/"};

GameScene::GameScene(std::string aName,
                     GameContext & aGameContext,
                     EntityWrap<component::MappingContext> & aContext,
                     EntHandle aSceneRoot) :
    Scene(aName, aGameContext, aContext, aSceneRoot),
    mTiles{mGameContext.mWorld},
    mRoundTransients{mGameContext.mWorld},
    mSlots{mGameContext.mWorld},
    mPlayers{mGameContext.mWorld},
    mPathfinders{mGameContext.mWorld}
{
    mGameContext.mLevelData = EntityWrap<component::LevelSetupData>(
        mGameContext.mWorld,
        component::LevelSetupData(
            mGameContext.mResources.find(gMarkovRoot).value(), "snaclvl4.xml",
            {Size3_i{19, 19, 1}}, 123123));
}

void GameScene::teardown(RawInput & aInput)
{
    TIME_SINGLE(Main, "teardown game scene");
    {
        Phase destroyPlayer;
        mPlayers.each(
            [&destroyPlayer](EntHandle aHandle, component::Controller &)
            { eraseEntityRecursive(aHandle, destroyPlayer); });
    }
    {
        Phase destroy;

        mStageDecor->get(destroy)->erase();

        mTiles.each([&destroy](EntHandle aHandle, const component::LevelTile &)
                    { aHandle.get(destroy)->erase(); });
        // Destroy round transient and components
        mRoundTransients.each(
            [&destroy](EntHandle aHandle, component::RoundTransient &)
            { aHandle.get(destroy)->erase(); });

        mGameContext.mLevel->get(destroy)->erase();

        mSystems.get(destroy)->erase();
        mSystems = mGameContext.mWorld.addEntity();
    }
    {
        // TODO: (franz) remove this at some point
        Phase debugDestroy;
        mPathfinders.each(
            [&debugDestroy](EntHandle aHandle, const component::PathToOnGrid &)
            { aHandle.get(debugDestroy)->erase(); });
    }
}

std::optional<Transition> GameScene::update(const snac::Time & aTime,
                                            RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    Phase update;

    bool quit = false;

    std::vector<int> boundControllers;

    mPlayers.each(
        [&](component::PlayerRoundData &, component::Controller & aController)
        {
        switch (aController.mType)
        {
        case ControllerType::Keyboard:
            aController.mInput = convertKeyboardInput(
                "player", aInput.mKeyboard, mContext->mKeyboardMapping);
            break;
        case ControllerType::Gamepad:
            aController.mInput = convertGamepadInput(
                "player", aInput.mGamepads.at(aController.mControllerId),
                mContext->mGamepadMapping);
            break;
        default:
            break;
        }

        boundControllers.push_back(aController.mControllerId);

        quit |= static_cast<bool>(aController.mInput.mCommand & gQuitCommand);
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
            GameInput input{.mCommand = gNoCommand};
            const bool controllerIsKeyboard =
                (int) controlIndex == gKeyboardControllerIndex;

            if (controllerIsKeyboard)
            {
                input = convertKeyboardInput("unbound", aInput.mKeyboard,
                                             mContext->mKeyboardMapping);
            }
            else
            {
                GamepadState & rawGamepad = aInput.mGamepads.at(controlIndex);
                input = convertGamepadInput("unbound", rawGamepad,
                                            mContext->mGamepadMapping);
            }

            if (input.mCommand & gJoin)
            {
                addPlayer(mGameContext, controlIndex,
                          controllerIsKeyboard ? ControllerType::Keyboard
                                               : ControllerType::Gamepad);
            }
        }
    }

    if (quit)
    {
        return Transition{.mTransitionName = "back"};
    }

    // This should be divided in four phases right now
    // The cleanup phase

    if (mGameContext.mLevel && mGameContext.mLevel->isValid()
        && mSystems.get(update)->get<system::RoundMonitor>().isRoundOver())
    {
        Phase cleanup;
        component::LevelSetupData & data = mGameContext.mLevelData->get();

        mSystems.get(update)->get<system::RoundMonitor>().updateRoundScore();
        // Destroy level tiles
        mSystems.get(update)
            ->get<system::LevelManager>()
            .destroyTilesEntities();

        data.mSeed += 1;

        // Destroy the level entity
        mGameContext.mLevel->get(cleanup)->erase();

        // Destroy round transient and components
        mRoundTransients.each(
            [&cleanup](EntHandle aHandle, component::RoundTransient &)
            { aHandle.get(cleanup)->erase(); });
        // Remove the player from the scene graph
    }

    if (!mGameContext.mLevel || !mGameContext.mLevel->isValid())
    {
        const component::LevelSetupData & data = mGameContext.mLevelData->get();
        mGameContext.mLevel =
            mSystems.get(update)->get<system::LevelManager>().createLevel(data);
        insertEntityInScene(*mGameContext.mLevel, mSceneRoot);
    }

    int playerJoinCount = mGameContext.mPlayerSlots.size();

    // TODO: (franz) maybe at some point it would be better to
    // spawn player between rounds
    // This however needs to be thought through (like if someone starts
    // a game alone, should we spawn someone during the round in that case)
    if (playerJoinCount > mPlayers.countMatches())
    {
        mSystems.get(update)->get<system::PlayerSpawner>().spawnPlayers();
    }
    // The creation of the level
    // Spawning the players
    //  if there is an unspawned player and is there room in the map
    //  spawn the player
    //  else
    //  put the player in a queue for the next round
    //  Add all spawned player to the scene graph
    // The game phase

    mSystems.get(update)->get<system::PlayerInvulFrame>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::AllowMovement>().update();
    mSystems.get(update)->get<system::ConsolidateGridMovement>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::MovementIntegration>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::AnimationManager>().update();
    mSystems.get(update)->get<system::AdvanceAnimations>().update(aTime);
    mSystems.get(update)->get<system::Pathfinding>().update();
    mSystems.get(update)->get<system::PortalManagement>().preGraphUpdate();
    mSystems.get(update)->get<system::Explosion>().update(aTime);

    mSystems.get(update)->get<system::SceneGraphResolver>().update();

    mSystems.get(update)->get<system::PortalManagement>().postGraphUpdate();
    mSystems.get(update)->get<system::PowerUpUsage>().update(aTime);
    mSystems.get(update)->get<system::EatPill>().update();

    mSystems.get(update)->get<system::Debug_BoundingBoxes>().update();

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
