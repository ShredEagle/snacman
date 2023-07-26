// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit:
// compile with /bigobj

#include "GameScene.h"

#include "snacman/EntityUtilities.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "snacman/simulations/snacgame/scene/PauseScene.h"
#include "snacman/simulations/snacgame/scene/DataScene.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"

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
#include "../component/PlayerGameData.h"
#include "../component/PlayerHud.h"
#include "../component/MenuItem.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h"
#include "../component/Portal.h"
#include "../component/PoseScreenSpace.h"
#include "../component/RigAnimation.h"
#include "../component/Spawner.h"
#include "../component/Speed.h"
#include "../component/Tags.h"
#include "../component/Text.h"
#include "../component/VisualModel.h"
#include "../Entities.h"
#include "../EntityWrap.h"
#include "../GameContext.h"
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../InputConstants.h"
#include "../scene/Scene.h"
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
#include <entity/EntityManager.h>
#include <map> // for opera...
#include <memory>
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

GameScene::GameScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gGameSceneName, aGameContext, aContext),
    mLevelData{mGameContext.mWorld,
               component::LevelSetupData(
                   mGameContext.mResources.find(gMarkovRoot).value(),
                   "snaclvl4.xml",
                   {Size3_i{19, 19, 1}},
                   123123)},
    mTiles{mGameContext.mWorld},
    mRoundTransients{mGameContext.mWorld},
    mSlots{mGameContext.mWorld},
    mHuds{mGameContext.mWorld},
    mPlayers{mGameContext.mWorld},
    mPathfinders{mGameContext.mWorld}
{
    // Preload models to avoid loading time when they first appear in the game
    mGameContext.mResources.getModel("models/collar/collar.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/teleport/teleport.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/missile/missile.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/boom/boom.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/portal/portal.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/dog/dog.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/missile/area.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/square_biscuit/square_biscuit.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/burger/burger.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/portal/portal.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/billpad/billpad.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/donut/donut.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/arrow/arrow.gltf",
                                     "effects/MeshTextures.sefx");
    {
        Phase init;
        mSystems.get(init)
            ->add(system::SceneGraphResolver{mGameContext,
                                             mGameContext.mSceneRoot})
            .add(system::InputProcessor{mGameContext, mMappingContext})
            .add(system::PlayerSpawner{mGameContext})
            .add(system::RoundMonitor{mGameContext})
            .add(system::PlayerInvulFrame{mGameContext})
            .add(system::Explosion{mGameContext})
            .add(system::AllowMovement{mGameContext})
            .add(system::ConsolidateGridMovement{mGameContext})
            .add(system::IntegratePlayerMovement{mGameContext})
            .add(system::LevelManager{mGameContext})
            .add(system::MovementIntegration{mGameContext})
            .add(system::AdvanceAnimations{mGameContext})
            .add(system::AnimationManager{mGameContext})
            .add(system::EatPill{mGameContext})
            .add(system::PortalManagement{mGameContext})
            .add(system::PowerUpUsage{mGameContext})
            .add(system::Pathfinding{mGameContext})
            .add(system::Debug_BoundingBoxes{mGameContext});
    }

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    snac::Orbital & camOrbital = camera.get()->get<snac::Orbital>();
    camOrbital.mSpherical.polar() = gInitialCameraSpherical.polar();
    camOrbital.mSpherical.radius() = gInitialCameraSpherical.radius();

    mSlots.each([this](EntHandle aHandle, const component::PlayerSlot &) {
        addBillpadHud(mGameContext, aHandle);
    });
}

void GameScene::onEnter(Transition aTransition)
{
    if (aTransition.mTransitionName == gQuitTransitionName)
    {
        mGameContext.mSceneStack->popScene();
        mGameContext.mSceneStack->pushScene(
            std::make_shared<MenuScene>(mGameContext, mMappingContext));
    }
}


void GameScene::onExit(Transition aTransition)
{
    TIME_SINGLE(Main, "teardown game scene");
    if (aTransition.mTransitionName == GameScene::sToPauseTransition)
    {
        return;
    }

    {
        Phase destroyPlayer;
        mSlots.each([&destroyPlayer](EntHandle aHandle, const component::PlayerSlot &)
                    { eraseEntityRecursive(aHandle, destroyPlayer); });
        // Delete hud
        mHuds.each([&destroyPlayer](EntHandle aHandle, const component::PlayerHud &)
                   { eraseEntityRecursive(aHandle, destroyPlayer); });
    }
    {
        Phase destroyEnvironment;

        eraseEntityRecursive(mLevel, destroyEnvironment);

        mSystems.get(destroyEnvironment)->erase();
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

void GameScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    Phase update;

    bool quit = false;

    std::vector<ControllerCommand> controllers =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "player", "unbound");

    for (auto controller : controllers)
    {
        if (controller.mBound)
        {
            quit |= controller.mInput.mCommand == gQuitCommand;
        }
        else
        {
            if (controller.mInput.mCommand & gJoin)
            {
                EntHandle slot = addPlayer(mGameContext, controller.mId);
                addBillpadHud(mGameContext, slot);
            }
        }
    }

    if (quit)
    {
        mGameContext.mSceneStack->pushScene(
            std::make_shared<PauseScene>(mGameContext, mMappingContext), Transition{.mTransitionName = GameScene::sToPauseTransition});
        return;
    }

    // This should be divided in four phases right now
    // The cleanup phase

    if (mLevel.isValid()
        && mSystems.get(update)->get<system::RoundMonitor>().isRoundOver())
    {
        component::LevelSetupData & data = *mLevelData;
        data.mSeed += 1;

        mSystems.get(update)->get<system::RoundMonitor>().updateRoundScore();

        std::vector<unsigned int> playerControllerIndices;
        {
            Phase cleanup;
            // This removes everything from the level players included
            eraseEntityRecursive(mLevel, cleanup);
        }
    }

    if (!mLevel.isValid())
    {
        const component::LevelSetupData & data = *mLevelData;
        mLevel =
            mSystems.get(update)->get<system::LevelManager>().createLevel(data);
        insertEntityInScene(mLevel, mGameContext.mSceneRoot);
    }

    auto level = snac::getComponent<component::Level>(mLevel);

    // TODO: (franz) maybe at some point it would be better to
    // spawn player between rounds
    // This however needs to be thought through (like if someone starts
    // a game alone, should we spawn someone during the round in that case)
    mSystems.get(update)->get<system::PlayerSpawner>().spawnPlayers(mLevel);
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
    mSystems.get(update)->get<system::AllowMovement>().update(level);
    mSystems.get(update)->get<system::ConsolidateGridMovement>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::MovementIntegration>().update(
        (float) aTime.mDeltaSeconds);
    mSystems.get(update)->get<system::AnimationManager>().update();
    mSystems.get(update)->get<system::AdvanceAnimations>().update(aTime);
    mSystems.get(update)->get<system::Pathfinding>().update(level);
    mSystems.get(update)->get<system::PortalManagement>().preGraphUpdate();
    mSystems.get(update)->get<system::Explosion>().update(aTime);

    mSystems.get(update)->get<system::SceneGraphResolver>().update();

    mSystems.get(update)->get<system::PortalManagement>().postGraphUpdate(level);
    mSystems.get(update)->get<system::PowerUpUsage>().update(aTime, mLevel);
    mSystems.get(update)->get<system::EatPill>().update();

    mSystems.get(update)->get<system::Debug_BoundingBoxes>().update();
}

} // namespace scene
} // namespace snacgame
} // namespace ad
