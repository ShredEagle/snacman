// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit:
// compile with /bigobj

#include "GameScene.h"

#include "math/Color.h"
#include "snac-renderer-V1/text/Text.h"
#include "snacman/EntityUtilities.h"
#include "snacman/simulations/snacgame/scene/DataScene.h"
#include "snacman/simulations/snacgame/scene/DisconnectedControllerScene.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "snacman/simulations/snacgame/scene/PauseScene.h"
#include "snacman/simulations/snacgame/system/FallingPlayers.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/system/TextZoom.h"

#include "../component/AllowedMovement.h"
#include "../component/Collision.h"
#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Explosion.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/LevelData.h"
#include "../component/MenuItem.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerHud.h"
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

constexpr const char * gMarkovRoot = {"markov/"};
constexpr int gTextSize = snac::gDefaultPixelHeight * 3;

GameScene::GameScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gGameSceneName, aGameContext, aContext),
    mLevelData{mGameContext.mWorld, "level data",
               component::LevelSetupData(
                   mGameContext.mResources.find(gMarkovRoot).value(),
                   "snaclvl4.xml",
                   {Size3_i{19, 19, 1}},
                   123123)},
    mSceneData{mGameContext.mWorld, "game scene data"},
    mTiles{mGameContext.mWorld},
    mRoundTransients{mGameContext.mWorld},
    mSlots{mGameContext.mWorld},
    mHuds{mGameContext.mWorld},
    mPlayers{mGameContext.mWorld},
    mPathfinders{mGameContext.mWorld}
{
    TIME_SINGLE(Main, "Constructor game scene");
    // Preload models to avoid loading time when they first appear in the game
    mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize);
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
    mGameContext.mResources.getModel(
        "models/square_biscuit/square_biscuit.gltf",
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
            .add(system::Debug_BoundingBoxes{mGameContext})
            .add(system::FallingPlayersSystem{mGameContext})
            .add(system::TextZoomSystem{mGameContext});
    }

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    renderer::Orbital & camOrbital =
        snac::getComponent<system::OrbitalCamera>(camera).mControl.mOrbital;
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
        mSlots.each(
            [&destroyPlayer](EntHandle aHandle, const component::PlayerSlot &) {
                eraseEntityRecursive(aHandle, destroyPlayer);
            });
        // Delete hud
        mHuds.each(
            [&destroyPlayer](EntHandle aHandle, const component::PlayerHud &) {
                eraseEntityRecursive(aHandle, destroyPlayer);
            });
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
        mPathfinders.each([&debugDestroy](EntHandle aHandle,
                                          const component::PathToOnGrid &) {
            aHandle.get(debugDestroy)->erase();
        });
    }
    {
        Phase destroyText;
        if (mReadyText.isValid())
        {
            mReadyText.get(destroyText)->erase();
        }
        if (mGoText.isValid())
        {
            mGoText.get(destroyText)->erase();
        }
        if (mVictoryText.isValid())
        {
            mVictoryText.get(destroyText)->erase();
        }
    }
}

math::hdr::Rgba_f getColorMixFromBitMask(char playerBitMask)
{
    std::array<math::hdr::Rgba_f, 4> resultColorList;
    char resultColorCount = 0;

    // Extract position from bitmask and fill the list
    // with the corresponding slot colors
    while(playerBitMask)
    {
        char position = 0;
        char tempMask = playerBitMask;
        while((tempMask & 1) == 0 && tempMask > 0)
        {
            position++;
            tempMask = tempMask >> (char)1;
        }

        playerBitMask ^= (char)1 << position;
        resultColorList.at(resultColorCount++) = gSlotColors.at(position);
    }

    // Average those colors and return
    math::hdr::Rgba_f resultColor = math::hdr::gBlack<float>;
    for (int i = 0; i < resultColorCount; i++)
    {
        resultColor += resultColorList.at(i) / (float)resultColorCount;
    }
    return resultColor;
}

char GameScene::findWinner()
{
    ent::Phase findWinner;
    char leaderBitMask = mSystems.get(findWinner)->get<system::RoundMonitor>().updateRoundScore();
    return leaderBitMask;
}

EntHandle GameScene::createSpawningPhaseText(const std::string & aText,
                                             const math::hdr::Rgba_f & aColor,
                                             const snac::Time & aTime)
{
    Phase phase;
    std::shared_ptr<snac::Font> boigaFont =
        mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize);

    math::Position<2, float> position = {0.f, 0.f};
    EntHandle textHandle =
        makeText(mGameContext, phase, aText, boigaFont, aColor, position);

    auto parameterAnimation =
        math::ParameterAnimation<float, math::AnimationResult::FullRange,
                                 math::None, math::ease::MassSpringDamper>(
            math::ease::MassSpringDamper<float>(0.09f, 10.f, 0.97f, 10.0f), 1.f,
            1.f);

    textHandle.get(phase)->add(
        component::TextZoom{snac::Clock::now(), parameterAnimation});

    return textHandle;
}

void GameScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    bool quit = false;

    std::vector<ControllerCommand> controllers =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "player", "unbound");
    std::array<int, 4> disconnectedControllerList;
    int disconnectedCount = 0;

    for (auto controller : controllers)
    {
        if (controller.mBound)
        {
            quit |= controller.mInput.mCommand == gQuitCommand;

            if (!controller.mConnected)
            {
                disconnectedControllerList.at(disconnectedCount++) =
                    controller.mId;
            }
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

    if (mSceneData->mPhase == GamePhase::NullPhase)
    {
        mSceneData->changePhase(GamePhase::SpawningSequence, aTime);
    }

    if (quit)
    {
        mGameContext.mSceneStack->pushScene(
            std::make_shared<PauseScene>(mGameContext, mMappingContext),
            Transition{.mTransitionName = sToPauseTransition});
        return;
    }

    if (disconnectedCount > 0)
    {
        mGameContext.mSceneStack->pushScene(
            std::make_shared<DisconnectedControllerScene>(mGameContext,
                                                          mMappingContext),
            Transition{.mTransitionName = sToPauseTransition,
                       .mSceneInfo = DisconnectedControllerInfo{
                           disconnectedControllerList}});
        return;
    }

    if (mLevel.isValid()
        && mSystems.get()->get<system::RoundMonitor>().isRoundOver()
        && mSceneData->mPhase != GamePhase::VictorySequence)
    {
        mSceneData->changePhase(GamePhase::VictorySequence, aTime);
    }

    // This is probably useful for everyone
    snac::Clock::duration elapsed =
        aTime.mTimepoint - mSceneData->mStartOfPhase;

    switch (mSceneData->mPhase)
    {
    case GamePhase::NullPhase:
        assert(false && "NullPhase should not happen");
        break;
    case GamePhase::SpawningSequence:
    {
        // Creating level from markov data
        if (!mLevel.isValid())
        {
            mLevel = mSystems.get()->get<system::LevelManager>().createLevel(
                *mLevelData);
            insertEntityInScene(mLevel, mGameContext.mSceneRoot);
            mSystems.get()
                ->get<system::PlayerSpawner>()
                .spawnPlayersBeforeRound(mLevel);
            std::array<std::pair<EntHandle, component::PlayerRoundData *>, 4> leaders;
            int leaderCount = 0;
            int leaderScore = 0;
            mPlayers.each(
                [&leaders, &leaderCount, &leaderScore](EntHandle aHandle,
                                         component::PlayerRoundData & aRoundData, const component::Controller & aController) {
                    component::PlayerGameData & gameData =
                        snac::getComponent<component::PlayerGameData>(aRoundData.mSlot);
                    if (!aRoundData.mCrown.isValid())
                    {
                        if (gameData.mRoundsWon > leaderScore)
                        {
                            leaderCount = 0;
                            leaders.at(leaderCount++) = {aHandle, &aRoundData};
                            leaderScore = gameData.mRoundsWon;
                        }
                        else if (gameData.mRoundsWon == leaderScore)
                        {
                            leaders.at(leaderCount++) = {aHandle, &aRoundData};
                        }
                    }
                });

            for (int i = 0; i < leaderCount; i++)
            {
                auto & [leaderEnt, leaderData] = leaders.at(i);
                EntHandle crown = createCrown(mGameContext);
                leaderData->mCrown = crown;
                insertEntityInScene(crown, leaderEnt);
                system::updateGlobalPosition(snac::getComponent<component::SceneNode>(crown));
            }
        }
        if (elapsed > snac::ms{1000} && !mReadyText.isValid()
            && !mGoText.isValid())
        {
            mReadyText = createSpawningPhaseText("Ready?",
                                                 math::hdr::gWhite<float>, aTime);
        }

        if (mReadyText.isValid())
        {
            auto zoomComp = snac::getComponent<component::TextZoom>(mReadyText);
            auto zoomAnim = zoomComp.mParameter;
            float duration = (float) snac::asSeconds(snac::Clock::now()
                                                     - zoomComp.mStartTime);
            if (zoomAnim.isCompleted(duration))
            {
                Phase destroy;
                mReadyText.get(destroy)->erase();
                mGoText = createSpawningPhaseText("Go!", math::hdr::gWhite<float>, aTime);
            }
        }

        if (mGoText.isValid())
        {
            auto zoomComp = snac::getComponent<component::TextZoom>(mGoText);
            auto zoomAnim = zoomComp.mParameter;
            float duration = (float) snac::asSeconds(snac::Clock::now()
                                                     - zoomComp.mStartTime);
            if (zoomAnim.isCompleted(duration))
            {
                Phase destroy;
                mGoText.get(destroy)->erase();
                mSceneData->changePhase(GamePhase::Playing, aTime);
                break;
            }
        }

        mSystems.get()->get<system::MovementIntegration>().update(
            (float) aTime.mDeltaSeconds);
        mSystems.get()->get<system::SceneGraphResolver>().update();
        mSystems.get()->get<system::FallingPlayersSystem>().update();
        mSystems.get()->get<system::TextZoomSystem>().update(aTime);
        break;
    }
    case GamePhase::VictorySequence:
    {
        if (elapsed
            < snac::ms{3000} / mGameContext.mSimulationControl.mSpeedRatio)
        {
            mGameContext.mSimulationControl.mSpeedRatio = 5;
            if (!mVictoryText.isValid())
            {
                char leaderBitMask = findWinner();
                auto color = getColorMixFromBitMask(leaderBitMask);
                mVictoryText = createSpawningPhaseText("Victory!", color, aTime);
            }
            auto level = snac::getComponent<component::Level>(mLevel);
            mSystems.get()->get<system::AllowMovement>().update(level);
            mSystems.get()->get<system::IntegratePlayerMovement>().update(
                (float) aTime.mDeltaSeconds);
            mSystems.get()->get<system::MovementIntegration>().update(
                (float) aTime.mDeltaSeconds);
            mSystems.get()->get<system::AnimationManager>().update();
            mSystems.get()->get<system::AdvanceAnimations>().update(aTime);
            mSystems.get()->get<system::Pathfinding>().update(level);
            mSystems.get()->get<system::PortalManagement>().preGraphUpdate();
            mSystems.get()->get<system::Explosion>().update(aTime);
            mSystems.get()->get<system::SceneGraphResolver>().update();
            mSystems.get()->get<system::PortalManagement>().postGraphUpdate(
                level);
            mSystems.get()->get<system::TextZoomSystem>().update(aTime);
        }
        else
        {
            component::LevelSetupData & data = *mLevelData;
            data.mSeed += 1;

            mSystems.get()->get<system::RoundMonitor>().updateRoundScore();

            // Transfer controller to slot before deleting player models
            {
                Phase transferController;
                mSlots.each(
                    [&](EntHandle aSlotHandle, component::PlayerSlot & aSlot) {
                        component::Controller & controller =
                            snac::getComponent<component::Controller>(
                                aSlot.mPlayer);
                        aSlotHandle.get(transferController)->add(controller);
                    });
            }

            Phase cleanup;
            // This removes everything from the level players models
            // included
            eraseEntityRecursive(mLevel, cleanup);
            mVictoryText.get(cleanup)->erase();
            mSceneData->changePhase(GamePhase::SpawningSequence, aTime);
            mGameContext.mSimulationControl.mSpeedRatio = 1;
            
        }
        break;
    }
    case GamePhase::Playing:
    {
        auto level = snac::getComponent<component::Level>(mLevel);

        mSystems.get()->get<system::PlayerInvulFrame>().update(
            (float) aTime.mDeltaSeconds);
        mSystems.get()->get<system::AllowMovement>().update(level);
        mSystems.get()->get<system::ConsolidateGridMovement>().update(
            (float) aTime.mDeltaSeconds);
        mSystems.get()->get<system::IntegratePlayerMovement>().update(
            (float) aTime.mDeltaSeconds);
        mSystems.get()->get<system::MovementIntegration>().update(
            (float) aTime.mDeltaSeconds);
        mSystems.get()->get<system::AnimationManager>().update();
        mSystems.get()->get<system::AdvanceAnimations>().update(aTime);
        mSystems.get()->get<system::Pathfinding>().update(level);
        mSystems.get()->get<system::PortalManagement>().preGraphUpdate();
        mSystems.get()->get<system::Explosion>().update(aTime);

        mSystems.get()->get<system::SceneGraphResolver>().update();

        mSystems.get()->get<system::PortalManagement>().postGraphUpdate(level);
        mSystems.get()->get<system::PowerUpUsage>().update(aTime, mLevel);
        mSystems.get()->get<system::EatPill>().update();

        mSystems.get()->get<system::Debug_BoundingBoxes>().update();

        break;
    }
    }
}

} // namespace scene
} // namespace snacgame
} // namespace ad
