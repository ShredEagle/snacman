#include "SnacGame.h"

#include "component/AllowedMovement.h"
#include "component/Context.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/LevelData.h"
#include "component/MenuItem.h"
#include "component/PathToOnGrid.h"
#include "component/PlayerGameData.h"
#include "component/PlayerHud.h"
#include "component/PlayerRoundData.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/RigAnimation.h"
#include "component/SceneNode.h"
#include "component/Tags.h"
#include "component/Text.h"
#include "component/VisualModel.h"
#include "Entities.h"
#include "entity/Entity.h"
#include "GameContext.h"
#include "GameParameters.h"
#include "ImguiInhibiter.h"
#include "InputConstants.h"
#include "math/Angle.h"
#include "scene/DataScene.h"
#include "scene/GameScene.h"
#include "scene/MenuScene.h"
#include "scene/Scene.h"
#include "SceneGraph.h"
#include "SimulationControl.h"
#include "snacman/EntityUtilities.h"
#include "snacman/serialization/Witness.h"
#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/MovementScreenSpace.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"
#include "snacman/simulations/snacgame/scene/JoinGameScene.h"
#include "system/SceneStack.h"
#include "system/SystemOrbitalCamera.h"
#include "typedef.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <handy/Guard.h>
#include <imgui.h>
#include <imguiui/ImguiUi.h>
#include <map>
#include <math/Color.h>
#include <memory>
#include <mutex>
#include <optional>
#include <snacman/DevmodeControl.h>
#include <snacman/ImguiUtilities.h>
#include <snacman/LoopSettings.h>
#include <snacman/Profiling.h>
#include <snacman/Profiling_V2.h>
#include <snacman/QueryManipulation.h>
#include <snacman/RenderThread.h>
#include <snacman/TemporaryRendererHelpers.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ad {
struct RawInput;

namespace snacgame {

/* namespace { */
/*     EntityWrap<SceneStack> createSceneStateMachine() */
/*     { */
/*     } */
/* } */

SnacGame::SnacGame(graphics::AppInterface & aAppInterface,
                   snac::RenderThread<Renderer_t> & aRenderThread,
                   imguiui::ImguiUi & aImguiUi,
                   resource::ResourceFinder aResourceFinder,
                   arte::Freetype & aFreetype,
                   RawInput & aInput) :
    mAppInterface{&aAppInterface},
    mGameContext(
        snac::Resources{std::move(aResourceFinder), aFreetype, aRenderThread},
        aRenderThread),
    mMappingContext{mGameContext.mWorld, mGameContext.mResources},
    mSystemOrbitalCamera{mGameContext.mWorld, mGameContext.mWorld,
                         math::getRatio<float>(mAppInterface->getWindowSize())},
    mQueryRenderable{mGameContext.mWorld, mGameContext.mWorld},
    mQueryTextWorld{mGameContext.mWorld, mGameContext.mWorld},
    mQueryTextScreen{mGameContext.mWorld, mGameContext.mWorld},
    mQueryHuds{mGameContext.mWorld, mGameContext.mWorld},
    mImguiUi{aImguiUi}
{
    ent::Phase init;

    /* // Add permanent game title */
    /* makeText(mGameContext, init, "Snacman", */
    /*          mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf"),
     */
    /*          math::hdr::gYellow<float>, {-0.25f, 0.75f}, {1.8f, 1.8f}); */

    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::StageDecorScene>(mGameContext,
                                                 mMappingContext));
    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::MenuScene>(mGameContext, mMappingContext));
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           RawInput & aInput,
                           const std::string & aProfilerResults)
{
    TIME_RECURRING_FUNC(Main);

    bool recompileShaders = false;
    {
        std::lock_guard lock{mImguiUi.mFrameMutex};
        mImguiUi.newFrame();
        // We must make sure to match each newFrame() to a render(),
        // otherwise we might get access violation in the RenderThread, when
        // calling ImguiUi::renderBackend()
        Guard renderGuard{[this]() { this->mImguiUi.render(); }};

        // NewFrame() updates the io catpure flag: consume them ASAP
        // see:
        // https://pixtur.github.io/mkdocs-for-imgui/site/FAQ/#qa-integration
        aInhibiter.resetCapture(static_cast<ImguiInhibiter::WantCapture>(
            (mImguiUi.isCapturingMouse() ? ImguiInhibiter::Mouse
                                         : ImguiInhibiter::Null)
            | (mImguiUi.isCapturingKeyboard() ? ImguiInhibiter::Keyboard
                                              : ImguiInhibiter::Null)));

        mImguiDisplays.display();

        if (mImguiDisplays.mShowSceneEditor)
        {
            mSceneEditor.showEditor(mGameContext.mSceneRoot);
        }
        scene::Scene * scene = mGameContext.mSceneStack->getActiveScene();
        if (mImguiDisplays.mShowRoundInfo && scene->mName == "game")
        {
            static ent::Query<component::Pill> pills{mGameContext.mWorld};
            ImGui::Begin("Round info", &mImguiDisplays.mShowPlayerInfo);
            if (ImGui::Button("next round"))
            {
                ent::Phase pillRemove;
                pills.each([&pillRemove](EntHandle aHandle, component::Pill &) {
                    aHandle.get(pillRemove)->erase();
                });
            }
            scene::GameScene * gameScene = (scene::GameScene *) scene;
            component::LevelSetupData levelData = *gameScene->mLevelData;
            ImGui::Text("%d", levelData.mSeed);
            ImGui::End();
        }
        if (mImguiDisplays.mShowPlayerInfo)
        {
            ImGui::Begin("Player Info", &mImguiDisplays.mShowPlayerInfo);
            static ent::Query<component::PlayerSlot> playerSlotQuery{
                mGameContext.mWorld};
            static ent::Query<component::Controller> controllerQuery{
                mGameContext.mWorld};
            playerSlotQuery.each([&](EntHandle aPlayer,
                                     const component::PlayerSlot &
                                         aPlayerSlot) {
                ImGui::PushID(aPlayerSlot.mSlotIndex);
                char playerHeader[64];
                std::snprintf(playerHeader, IM_ARRAYSIZE(playerHeader),
                              "Player %d", aPlayerSlot.mSlotIndex + 1);
                if (ImGui::CollapsingHeader(playerHeader))
                {
                    // This is an assumption but currently player that have
                    // a geometry should have a globalPose
                    if (aPlayer.get()->has<component::Geometry>())
                    {
                        ent::Entity_view player = *aPlayer.get();
                        const component::Geometry & geo =
                            player.get<component::Geometry>();
                        const component::Controller & controller =
                            player.get<component::Controller>();
                        const component::GlobalPose & pose =
                            player.get<component::GlobalPose>();

                        ImGui::BeginChild("Action", ImVec2(200.f, 0.f), true);
                        if (controller.mType == ControllerType::Dummy)
                        {
                            if (ImGui::Button("Bind to keyboard"))
                            {
                                EntHandle oldController = snac::getFirstHandle(
                                    controllerQuery,
                                    [](const component::Controller &
                                           aController) {
                                        return aController.mType
                                               == ControllerType::Keyboard;
                                    });

                                if (oldController.isValid())
                                {
                                    oldController.get()
                                        ->get<component::Controller>()
                                        .mType = ControllerType::Dummy;
                                    oldController.get()
                                        ->get<component::Controller>()
                                        .mControllerId = aPlayerSlot.mSlotIndex;
                                }
                                aPlayer.get()
                                    ->get<component::Controller>()
                                    .mType = ControllerType::Keyboard;
                                aPlayer.get()
                                    ->get<component::Controller>()
                                    .mControllerId = gKeyboardControllerIndex;
                            }

                            if (ImGui::Button("Remove player"))
                            {
                                Phase update;
                                eraseEntityRecursive(aPlayer, update);
                            }
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                        ImGui::BeginChild("Info");
                        geo.drawUi();
                        pose.drawUi();
                        controller.drawUi();
                        ImGui::EndChild();
                    }
                    else
                    {
                        if (ImGui::Button("Create dummy player"))
                        {
                            {
                                /* addPlayer(mGameContext, aPlayerSlot.mIndex,
                                 */
                                /*           ControllerType::Dummy); */
                            }
                        }
                    }
                }
                ImGui::PopID();
            });
            ImGui::End();
        }
        if (mImguiDisplays.mShowLogLevel)
        {
            snac::imguiLogLevelSelection(&mImguiDisplays.mShowLogLevel);
        }
        if (mImguiDisplays.mShowDebugDrawers)
        {
            snac::imguiDebugDrawerLevelSection(
                &mImguiDisplays.mShowDebugDrawers);
        }
        if (mImguiDisplays.mShowMappings)
        {
            mMappingContext->drawUi(aInput, &mImguiDisplays.mShowMappings);
        }
        if (mImguiDisplays.mShowImguiDemo)
        {
            ImGui::ShowDemoWindow(&mImguiDisplays.mShowImguiDemo);
        }
        if (mImguiDisplays.mSpeedControl)
        {
            mGameContext.mSimulationControl.drawSimulationUi(
                mGameContext.mWorld, &mImguiDisplays.mSpeedControl);
        }
        if (mImguiDisplays.mShowSimulationDelta)
        {
            ImGui::Begin("Gameloop", &mImguiDisplays.mShowSimulationDelta);
            int simulationDelta =
                (int) duration_cast<std::chrono::milliseconds>(
                    aSettings.mSimulationPeriod)
                    .count();
            if (ImGui::InputInt("Update call period (ms)", &simulationDelta))
            {
                simulationDelta = std::clamp(simulationDelta, 1, 200);
            }
            aSettings.mSimulationPeriod =
                std::chrono::milliseconds{simulationDelta};
            int updateDuration = (int) duration_cast<std::chrono::milliseconds>(
                                     aSettings.mUpdateDuration)
                                     .count();
            if (ImGui::InputInt("Update duration (ms)", &updateDuration))
            {
                updateDuration = std::clamp(updateDuration, 1, 500);
            }
            aSettings.mUpdateDuration =
                std::chrono::milliseconds{updateDuration};

            bool interpolate = aSettings.mInterpolate;
            ImGui::Checkbox("State interpolation", &interpolate);
            aSettings.mInterpolate = interpolate;
            ImGui::End();
        }

        if (mImguiDisplays.mShowMainProfiler)
        {
            {
                ImGui::Begin("Main profiler",
                             &mImguiDisplays.mShowMainProfiler);
                std::string str;
                snac::getProfiler(snac::Profiler::Main).print(str);
                ImGui::TextUnformatted(str.c_str());
                ImGui::End();
            }

            ImGui::Begin("Main profiler V2", &mImguiDisplays.mShowMainProfiler);
            ImGui::TextUnformatted(aProfilerResults.c_str());
            ImGui::End();
        }
        if (mImguiDisplays.mShowRenderProfiler)
        {
            ImGui::Begin("Render profiler",
                         &mImguiDisplays.mShowRenderProfiler);
            ImGui::TextUnformatted(
                snac::getRenderProfilerPrint().get().c_str());
            ImGui::End();

            ImGui::Begin("Render profiler V2",
                         &mImguiDisplays.mShowRenderProfiler);
            ImGui::TextUnformatted(
                snac::v2::getRenderProfilerPrint().get().c_str());
            ImGui::End();
        }
        if (mImguiDisplays.mShowRenderControls)
        {
            // TODO This should be an easier raii object
            ImGui::Begin("Render controls",
                         &mImguiDisplays.mShowRenderControls);
            Guard endGuard{[]() { ImGui::End(); }};
            recompileShaders = ImGui::Button("Recompile shaders");

            mGameContext.mRenderThread.continueGui();
        }
        if (mImguiDisplays.mGameControl)
        {
            ImGui::Begin("Game control", &mImguiDisplays.mGameControl);
            if (ImGui::Button("End round"))
            {
                Phase destroy;
                ent::Query<component::Pill>{mGameContext.mWorld}.each(
                    [&destroy](EntHandle aHandle, const component::Pill &) {
                        aHandle.get(destroy)->erase();
                    });
            }
            ImGui::End();
        }
        if (mImguiDisplays.mPathfinding)
        {
            ImGui::Begin("Debug function", &mImguiDisplays.mPathfinding);
            if (ImGui::Button("Create pathfinder"))
            {
                EntHandle pathfinder = mGameContext.mWorld.addEntity();
                EntHandle player = snac::getFirstHandle(
                    ent::Query<component::PlayerSlot, component::Geometry>{
                        mGameContext.mWorld});
                if (player.isValid())
                {
                    Phase pathfinding;
                    Entity pEntity = *pathfinder.get(pathfinding);
                    assert(false
                           && "Sorry, I removed support for cube special case");
                    addMeshGeoNode(mGameContext, pEntity, "CUBE",
                                   "effects/Mesh.sefx", {7.f, 7.f, gPillHeight},
                                   1.f, {0.5f, 0.5f, 0.5f});
                    pEntity.add(component::AllowedMovement{
                        .mWindow = gOtherTurningZoneHalfWidth});
                    pEntity.add(component::PathToOnGrid{player});
                }
                EntHandle root = mGameContext.mSceneRoot;
                Phase phase;
                EntHandle level =
                    snac::getComponent<component::SceneNode>(root).mFirstChild;
                insertEntityInScene(pathfinder, level);
            }
            ImGui::End();
        }
        // TODO(franz): Remove this after serialization is over
        if (mImguiDisplays.mTestSerialization)
        {
            ImGui::SetNextWindowSize({800.f, 400.f}, ImGuiCond_Appearing);
            ImGui::Begin("test serialization", &mImguiDisplays.mTestSerialization);
            Guard testGuard{[]() { ImGui::End(); }};
            static std::string resultJsonDump;
            static std::string otherResultJsonDump;
            if (ImGui::Button("Launch test"))
            {
                auto handle = mGameContext.mWorld.addEntity("test serial");
                auto other = mGameContext.mWorld.addEntity("other test serial");
                {
                    ent::Phase phase;
                    handle.get(phase)->add(
                        component::Collision{component::gPillHitbox});
                    handle.get(phase)->add(component::AllowedMovement{1});
                    handle.get(phase)->add(component::Controller{
                        ControllerType::Gamepad,
                        {1, {0.23f, -0.12f}, {0.1f, 0.2f}},
                        1});
                    handle.get(phase)->add(component::Explosion{
                        snac::Clock::now(),
                        math::ParameterAnimation<
                            float, math::AnimationResult::Clamp>(0.5f)});
                    handle.get(phase)->add(component::Geometry{
                        {0.1f, 0.2f, 0.3f}, 0.9f, {0.9f, 0.8f, 0.7f}});
                    handle.get(phase)->add(component::GlobalPose{
                        {0.1f, 0.2f, 0.3f}, 0.9f, {0.9f, 0.8f, 0.7f}});
                    handle.get(phase)->add(
                        component::Tile{component::TileType::Portal,
                                        gAllowedMovementDown,
                                        {0.1f, 0.2f}});
                    handle.get(phase)->add(component::PathfindNode{
                        0.1f, 0.2f, nullptr, 1, {0.1f, 0.2f}, true, false});
                    handle.get(phase)->add(component::LevelSetupData{
                        "hello", "ouais", {{1, 1, 1}, {2, 2, 2}}, 123});
                    handle.get(phase)->add(component::MenuItem{
                        "yoyo",
                        false,
                        {{0, "Start"}},
                        scene::Transition{
                            .mTransitionName =
                                scene::JoinGameScene::sFromMenuTransition,
                            .mSceneInfo = scene::JoinGameSceneInfo{0}}});
                    handle.get(phase)->add(component::MovementScreenSpace{
                        math::Radian<float>{0.2f}});
                    handle.get(phase)->add(
                        component::PathToOnGrid{other, {1.1f, 1.2f}, false});
                    handle.get(phase)->add(
                        component::PlayerGameData{12, other});
                    handle.get(phase)->add(
                        component::PlayerHud{other, other, other});
                    handle.get(phase)->add(component::PlayerRoundData{});
                    handle.get(phase)->add(component::PoseScreenSpace{
                        {2.1f, 3.f}, {1.1f, 1.2f}, math::Radian<float>{0.f}});
                    // other = createPill(mGameContext, phase, {0.f, 0.f});
                    // handle.get(phase)->add(component::Text{
                    //     .mString = "ouais",
                    //     .mFont = mGameContext.mResources.getFont(
                    //         "fonts/notes/Bitcheese.ttf"),
                    //     .mColor = math::hdr::gBlack<float>,
                    // });
                }
                json result;
                witness_json(handle,
                             serial::Witness::make(&result, mGameContext));
                resultJsonDump = result.dump(2);
                json otherResult;
                witness_json(other,
                             serial::Witness::make(&otherResult, mGameContext));
                auto reader = mGameContext.mWorld.addEntity("test deserialize");
                json checkResult;
                testify_json(
                    reader, serial::Witness::make_const(&result, mGameContext));
                witness_json(reader,
                             serial::Witness::make(&checkResult, mGameContext));
                otherResultJsonDump = checkResult.dump(2);
                {
                    Phase phase;
                    handle.get(phase)->erase();
                    other.get(phase)->erase();
                    reader.get(phase)->erase();
                }
            }
            ImGui::SameLine();
            ImVec2 size = ImVec2((ImGui::GetContentRegionAvail().x
                                  - 1 * ImGui::GetStyle().ItemSpacing.x),
                                 0.f);
            ImGui::BeginChild("result", size, true);
            ImGui::Text("%s", resultJsonDump.c_str());
            ImGui::SameLine();
            ImGui::Text("%s", otherResultJsonDump.c_str());
            ImGui::EndChild();
        }
    }

    if (recompileShaders)
    {
        mGameContext.mRenderThread.recompileShaders(mGameContext.mResources)
            .get(); // so any exception is rethrown in this context
    }
}

bool SnacGame::update(snac::Clock::duration & aUpdatePeriod, RawInput & aInput)
{
    snac::DebugDrawer::StartFrame();

    if constexpr (snac::isDevmode())
    {
        mSystemOrbitalCamera->update(
            aInput,
            snac::Camera::gDefaults.vFov, // TODO Should be dynamic
            mAppInterface->getWindowSize().height());
    }

    // If the simulation is paused and step was not pressed, return now
    // This avoids incrementing the simulation time, and does not call the
    // systems update
    if (!mGameContext.mSimulationControl.mPlaying
        && !mGameContext.mSimulationControl.mStep)
    {
        return false;
    }

    mSimulationTime.advance(aUpdatePeriod
                            / mGameContext.mSimulationControl.mSpeedRatio);

    ent::Wrap<system::SceneStack> & sceneStack = mGameContext.mSceneStack;
    sceneStack->getActiveScene()->update(mSimulationTime, aInput);

    if (mGameContext.mSceneStack->empty())
    {
        return true;
    }

    if (mGameContext.mSimulationControl.mSaveGameState)
    {
        TIME_RECURRING(Main, "Save state");
        mGameContext.mSimulationControl.saveState(
            mGameContext.mWorld.saveState(),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                aUpdatePeriod),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                mSimulationTime.mDeltaDuration));
    }

    return false;
}

std::unique_ptr<Renderer_t::GraphicState_t> SnacGame::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<Renderer_t::GraphicState_t>();

    ent::Phase nomutation;

    //
    // Worldspace models
    //
    mQueryRenderable.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       const component::GlobalPose & aGlobPose,
                       // TODO #anim restore this constness (for the moment,
                       // animation mutate the rig's scene)
                       /*const*/ component::VisualModel & aVisualModel) {
            // Note: This feels bad to test component presence here
            // but we do not want to handle VisualModel the same way
            // depending on the presence of RigAnimation (and we do not have
            // "negation" on Queries, to separately get VisualModels without
            // RigAnimation)
            visu_V2::Entity::SkeletalAnimation skeletal;
            if (auto entity = *aHandle.get();
                entity.has<component::RigAnimation>())
            {
                const auto & rigAnimation =
                    aHandle.get()->get<component::RigAnimation>();
                skeletal = visu_V2::Entity::SkeletalAnimation{
                    .mAnimation = rigAnimation.mAnimation,
                    .mParameterValue = rigAnimation.mParameterValue,
                };
            }

            state->mEntities.insert(
                aHandle.id(),
                visu_V2::Entity{
                    .mColor = aGlobPose.mColor,
                    .mInstance =
                        renderer::Instance{
                            .mObject = aVisualModel.mModel,
                            .mPose =
                                renderer::Pose{
                                    .mPosition =
                                        aGlobPose.mPosition.as<math::Vec>(),
                                    .mUniformScale = aGlobPose.mScaling,
                                    .mOrientation = aGlobPose.mOrientation,
                                },
                            .mName = aHandle.name(),
                        },
                    .mInstanceScaling = aGlobPose.mInstanceScaling,
                    .mAnimationState = std::move(skeletal),
                    .mDisableInterpolation = aVisualModel.mDisableInterpolation,
                });
            // Note: Although it does not feel correct, this is a convenient
            // place to reset this flag
            aVisualModel.mDisableInterpolation = false;
        });

    //
    // Worldspace Text
    //
    mQueryTextWorld.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       component::Text & aText,
                       component::GlobalPose & aGlobPose) {
            state->mTextWorldEntities.insert(
                aHandle.id(), visu_V1::Text{
                                  .mPosition_world = aGlobPose.mPosition,
                                  // TODO remove the hardcoded value of 100
                                  // Note hardcoded 100 scale down. Because I'd
                                  // like a value of 1 for the scale of the
                                  // component to still mean "about visible".
                                  .mScaling = aGlobPose.mInstanceScaling
                                              * aGlobPose.mScaling / 100.f,
                                  .mOrientation = aGlobPose.mOrientation,
                                  .mString = aText.mString,
                                  .mFont = aText.mFont,
                                  .mColor = aText.mColor,
                              });
        });

    //
    // Screenspace Text
    //
    mQueryTextScreen.get(nomutation)
        .each([&state, this](ent::Handle<ent::Entity> aHandle,
                             component::Text & aText,
                             component::PoseScreenSpace & aPose) {
            math::Position<3, float> position_screenPix{
                aPose.mPosition_u.cwMul(
                    // TODO this multiplication should be done once and
                    // cached but it should be refreshed on framebuffer
                    // resizing.
                    static_cast<math::Position<2, GLfloat>>(
                        this->mAppInterface->getFramebufferSize())
                    / 2.f),
                0.f};

            state->mTextScreenEntities.insert(
                aHandle.id(),
                visu_V1::Text{
                    .mPosition_world = position_screenPix,
                    .mScaling = math::Size<3, float>{aPose.mScale, 1.f},
                    .mOrientation =
                        math::Quaternion{
                            math::UnitVec<3, float>::MakeFromUnitLength(
                                {0.f, 0.f, 1.f}),
                            aPose.mRotationCCW},
                    .mString = aText.mString,
                    .mFont = aText.mFont,
                    .mColor = aText.mColor,
                });
        });

    state->mCamera = mSystemOrbitalCamera->getCamera();

    state->mDebugDrawList = snac::DebugDrawer::EndFrame();

    return state;
}

} // namespace snacgame
} // namespace ad
