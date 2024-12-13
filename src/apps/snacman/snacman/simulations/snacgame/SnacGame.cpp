#include "SnacGame.h"

#include "ModelInfos.h"
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
#include "snacman/serialization/Serial.h"
#include "snacman/serialization/Witness.h"
#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/MovementScreenSpace.h"
#include "snacman/simulations/snacgame/component/PlayerJoinData.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"
#include "snacman/simulations/snacgame/scene/JoinGameScene.h"
#include "system/SceneStack.h"
#include "system/SystemOrbitalCamera.h"
#include "typedef.h"

#include <implot.h>
#include <snacman/DevmodeControl.h>
#include <snacman/LoopSettings.h>
#include <snacman/Profiling.h>
#include <snacman/Profiling_V2.h>
#include <snacman/QueryManipulation.h>
#include <snacman/RenderThread.h>
#include <snacman/TemporaryRendererHelpers.h>

#include <utilities/ImguiUtilities.h>

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
        aRenderThread, aAppInterface),
    // TODO Ad 2024/12/12: Should we also wrap the environment loading through Resources system?
    // Currently, quick direct loading of a static environment.
    mEnvironmentMap{aRenderThread.loadEnvironment(*mGameContext.mResources.find(gEnvMap)).get()},
    mMappingContext{mGameContext.mWorld, "mapping context", mGameContext.mResources},
    mSystemOrbitalCamera{mGameContext.mWorld, "orbital camera", mGameContext.mWorld,
                         math::getRatio<float>(mAppInterface->getWindowSize())},
    mMainLight{mGameContext.mWorld, "main light"},
    mLightAnimationSystem{mGameContext.mWorld, "light animation system", mGameContext},
    mQueryRenderable{mGameContext.mWorld, "query renderable", mGameContext.mWorld},
    mQueryTextWorld{mGameContext.mWorld, "query text world", mGameContext.mWorld},
    mQueryTextScreen{mGameContext.mWorld, "query text screen", mGameContext.mWorld},
    mQueryHuds{mGameContext.mWorld, "query huds", mGameContext.mWorld},
    mQueryLightDirections{mGameContext.mWorld, "query light directions", mGameContext.mWorld},
    mQueryLightPoints{mGameContext.mWorld, "query light points", mGameContext.mWorld},
    mImguiUi{aImguiUi},
    mDestroyPlotContext{[]() { ImPlot::DestroyContext(); }}
{
    ent::Phase init;
    ImPlot::CreateContext();

    /* // Add permanent game title */
    /* makeText(mGameContext, init, "Snacman", */
    /*          mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf"),
     */
    /*          math::hdr::gYellow<float>, {-0.25f, 0.75f}, {1.8f, 1.8f}); */
    mGameContext.mResources.getBlueprint("blueprints/decor.json", mGameContext.mWorld, mGameContext);

    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::StageDecorScene>(mGameContext,
                                                 mMappingContext),
        {.mTransitionType = snacgame::scene::TransType::FirstLaunch});
    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::MenuScene>(mGameContext, mMappingContext));

    // Initial setup of the directional main light
    *mMainLight = component::LightDirection{
        .mDirection = math::UnitVec<3, GLfloat>{
            math::Vec<3, GLfloat>{0.f, 0.f, -1.f} 
            * math::trans3d::rotateX(-math::Degree<float>{60.f})
            * math::trans3d::rotateY(math::Degree<float>{20.f})
        },
        .mColors{
            .mDiffuse = math::hdr::gWhite<float> * 0.8f,
            .mSpecular = math::hdr::gWhite<float> * 0.8f,
        },
        .mProjectShadow = true,
    };
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
            if (ImGui::Button("add player"))
            {
                EntHandle slotHandle = addPlayer(mGameContext, (int)playerSlotQuery.countMatches() + 3);
                const component::PlayerSlot & slot =
                    slotHandle.get()->get<component::PlayerSlot>();
                EntHandle joinGamePlayer = createJoinGamePlayer(mGameContext, slotHandle, (int)slot.mSlotIndex);
                insertEntityInScene(joinGamePlayer, mGameContext.mWorld.handleFromName("join game root"));
                Phase joinPlayer;
                slotHandle.get(joinPlayer)->add(component::PlayerJoinData{.mJoinPlayerModel = joinGamePlayer});
            }
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
            ImGui::Begin("Logging", &mImguiDisplays.mShowLogLevel);
            utilities::imguiLogLevelSelection();
            ImGui::End();
        }
        if (mImguiDisplays.mShowDebugDrawers)
        {
            ImGui::Begin("Debug drawing", &mImguiDisplays.mShowDebugDrawers);
            utilities::imguiDebugDrawerLevelSection();
            ImGui::End();
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
        if (mImguiDisplays.mShowEntities)
        {
            ImGui::Begin("data");
                Guard testGuard{[]() { ImGui::End(); }};
            witness_imgui(mGameContext.mWorld, serial::Witness::make(&mImguiUi, mGameContext));
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

    mLightAnimationSystem->update((float)mSimulationTime.mDeltaSeconds);

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

    mGameContext.mSoundManager.update();

    return false;
}

std::unique_ptr<Renderer_t::GraphicState_t> SnacGame::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<Renderer_t::GraphicState_t>();

    //
    // Worldspace models
    //
    mQueryRenderable
        ->each([&state](ent::Handle<ent::Entity> aHandle,
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
    // Lights
    //
    static constexpr math::hdr::Rgb_f ambientColor = math::hdr::Rgb_f{0.4f, 0.4f, 0.4f};

    state->mLights = renderer::LightsDataUi{
        renderer::LightsDataCommon{
            .mAmbientColor = ambientColor,
        },
    };

    mQueryLightDirections
        ->each([&state](const component::LightDirection & aLightDirection)
            {
                GLuint lightIdx = state->mLights.mDirectionalCount++;
                state->mLights.mDirectionalLights[lightIdx] =
                    renderer::DirectionalLight{
                        .mDirection = aLightDirection.mDirection,
                        .mDiffuseColor = aLightDirection.mColors.mDiffuse,
                        .mSpecularColor = aLightDirection.mColors.mSpecular,
                    };
                state->mLights.mDirectionalLightProjectShadow[lightIdx].mIsProjectingShadow = 
                    aLightDirection.mProjectShadow;
            });

    mQueryLightPoints
        ->each([&state](const component::GlobalPose & aGlobalPose,
                        const component::LightPoint & aLightPoint)
            {
                GLuint lightIdx = state->mLights.mPointCount++;
                state->mLights.mPointLights[lightIdx] =
                    renderer::PointLight{
                        .mPosition = aGlobalPose.mPosition,
                        .mRadius = aLightPoint.mRadius,
                        .mDiffuseColor = aLightPoint.mColors.mDiffuse,
                        .mSpecularColor = aLightPoint.mColors.mSpecular,
                    };
            });

    //
    // Worldspace Text
    //
    mQueryTextWorld
        ->each([&state](ent::Handle<ent::Entity> aHandle,
                       component::Text & aText,
                       // Taken by value to mutate it locally.
                       component::GlobalPose aGlobPose) {
            // TODO remove the hardcoded value of 100
            // Note hardcoded 100 scale down. Because I'd
            // like a value of 1 for the scale of the
            // component to still mean "about visible". 
            aGlobPose.mScaling /= 100;

            state->mTextWorldEntities.insert(
                aHandle.id(), visu_V2::Text{
                                  .mString = aText.mString,
                                  .mPose = toPose(aGlobPose),
                                  .mColor = aText.mColor,
                                  .mFontRef = aText.mFontRef,
                              });
        });

    //
    // Screenspace Text
    //
    mQueryTextScreen
        ->each([&state, this](ent::Handle<ent::Entity> aHandle,
                             component::Text & aText,
                             component::PoseScreenSpace & aPose)
        {
            math::Position<3, float> position_screenPix{
                aPose.mPosition_u.cwMul(
                    // TODO this multiplication should be done once and
                    // cached but it should be refreshed on framebuffer
                    // resizing.
                    // TODO Ad 2024/11/15: Actually, there should be a convenient way
                    // to express position of Screen space objects in [-1, 1]^2 directly.
                    static_cast<math::Position<2, GLfloat>>(
                        this->mAppInterface->getFramebufferSize())
                    / 2.f),
                0.f
        };

            // You know I like my scales uniform
            assert(aPose.mScale[0] == aPose.mScale[1]);

            state->mTextScreenEntities.insert(
                aHandle.id(),
                visu_V2::Text{
                    .mString = aText.mString,
                    .mPose = {
                        .mPosition = position_screenPix.as<math::Vec>(),
                        .mUniformScale = aPose.mScale[0],
                        .mOrientation = math::Quaternion{
                            math::UnitVec<3, float>::MakeFromUnitLength(
                            {0.f, 0.f, 1.f}),
                            aPose.mRotationCCW},
                    },
                    .mColor = aText.mColor,
                    .mFontRef = aText.mFontRef,
                });
        });

    //
    // Camera
    //
    state->mCamera = mSystemOrbitalCamera->getCamera();

    //
    // Environment map
    //
    // TODO Ad 2024/12/12: #renderer_API This is currently wasteful 
    // (makes a copy of the static environment each update)
    // It feels like a more generic interface is also lying here 
    // (currently, the environment is a RepositoryTexture)
    state->mEnvironment = mEnvironmentMap;

    //
    // Debug drawing
    //
    state->mDebugDrawList = snac::DebugDrawer::EndFrame();

    return state;
}

} // namespace snacgame
} // namespace ad
