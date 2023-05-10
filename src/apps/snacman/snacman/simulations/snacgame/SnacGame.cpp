#include "SnacGame.h"

#include "Entities.h"
#include "GameContext.h"
#include "GameParameters.h"
#include "InputConstants.h"
#include "scene/Scene.h"
#include "SimulationControl.h"
#include "SceneGraph.h"
#include "system/SceneStateMachine.h"
#include "system/SystemOrbitalCamera.h"
#include "typedef.h"

#include "component/AllowedMovement.h"
#include "component/Context.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/LevelTags.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerSlot.h"
#include "component/PathToOnGrid.h"
#include "component/PoseScreenSpace.h"
#include "component/RigAnimation.h"
#include "component/SceneNode.h"
#include "component/Text.h"
#include "component/VisualModel.h"

#include <snacman/ImguiUtilities.h>
#include <snacman/LoopSettings.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <handy/Guard.h>
#include <imgui.h>
#include <imguiui/ImguiUi.h>
#include <map>
#include <math/Color.h>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ad {
struct RawInput;

namespace snac {
template <class T_renderer>
class RenderThread;
}
namespace snacgame {

std::array<math::hdr::Rgba_f, 4> gSlotColors{
    math::hdr::Rgba_f{0.725f, 1.f, 0.718f, 1.f},
    math::hdr::Rgba_f{0.949f, 0.251f, 0.506f, 1.f},
    math::hdr::Rgba_f{0.961f, 0.718f, 0.f, 1.f},
    math::hdr::Rgba_f{0.f, 0.631f, 0.894f, 1.f},
};

SnacGame::SnacGame(graphics::AppInterface & aAppInterface,
                   snac::RenderThread<Renderer_t> & aRenderThread,
                   imguiui::ImguiUi & aImguiUi,
                   resource::ResourceFinder aResourceFinder,
                   arte::Freetype & aFreetype,
                   RawInput & aInput) :
    mAppInterface{&aAppInterface},
    mGameContext{
        .mResources = snac::Resources{std::move(aResourceFinder), aFreetype,
                                      aRenderThread},
        .mRenderThread = aRenderThread,
    },
    mMappingContext{mGameContext.mWorld, mGameContext.mResources},
    mStateMachine{
        mGameContext.mWorld, mGameContext.mWorld,
        *mGameContext.mResources.find("scenes/scene_description.json"),
        mMappingContext, mGameContext},
    mSystemOrbitalCamera{mGameContext.mWorld, mGameContext.mWorld},
    mQueryRenderable{mGameContext.mWorld, mGameContext.mWorld},
    mQueryTextWorld{mGameContext.mWorld, mGameContext.mWorld},
    mQueryTextScreen{mGameContext.mWorld, mGameContext.mWorld},
    mImguiUi{aImguiUi}
{
    ent::Phase init;

    // Creating the slot entity those will be used a player entities
    mGameContext.mWorld.addEntity().get(init)->add(
        component::PlayerSlot{0, false, gSlotColors.at(0)});
    mGameContext.mWorld.addEntity().get(init)->add(
        component::PlayerSlot{1, false, gSlotColors.at(1)});
    mGameContext.mWorld.addEntity().get(init)->add(
        component::PlayerSlot{2, false, gSlotColors.at(2)});
    mGameContext.mWorld.addEntity().get(init)->add(
        component::PlayerSlot{3, false, gSlotColors.at(3)});

    scene::Scene * scene = mStateMachine->getCurrentScene();
    scene->setup(scene::Transition{}, aInput);
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           RawInput & aInput)
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

        if (mImguiDisplays.mShowPlayerInfo)
        {
            ent::Phase update;
            static ent::Query<component::PlayerSlot> playerSlotQuery{
                mGameContext.mWorld};
            static ent::Query<component::Controller> controllerQuery{
                mGameContext.mWorld};
            ImGui::Begin("Player Info", &mImguiDisplays.mShowPlayerInfo);
            playerSlotQuery.each([&](EntHandle aPlayer,
                                     const component::PlayerSlot &
                                         aPlayerSlot) {
                ImGui::PushID(aPlayerSlot.mIndex);
                char playerHeader[64];
                std::snprintf(playerHeader, IM_ARRAYSIZE(playerHeader),
                              "Player %d", aPlayerSlot.mIndex + 1);
                if (ImGui::CollapsingHeader(playerHeader))
                {
                    // This is an assumption but currently player that have
                    // a geometry should have a globalPose and a PlayerMoveState
                    if (aPlayer.get(update)->has<component::Geometry>())
                    {
                        Entity player = *aPlayer.get(update);
                        const component::Geometry & geo =
                            player.get<component::Geometry>();
                        const component::Controller & controller =
                            player.get<component::Controller>();
                        const component::GlobalPose & pose =
                            player.get<component::GlobalPose>();
                        const component::PlayerMoveState & moveState =
                            player.get<component::PlayerMoveState>();

                        ImGui::BeginChild("Action", ImVec2(200.f, 0.f), true);
                        if (controller.mType == ControllerType::Dummy)
                        {
                            if (ImGui::Button("Bind to keyboard"))
                            {
                                OptEntHandle oldController =
                                    snac::getFirstHandle(
                                        controllerQuery,
                                        [](const component::Controller &
                                               aController) {
                                            return aController.mType
                                                   == ControllerType::Keyboard;
                                        });

                                if (oldController)
                                {
                                    oldController->get(update)
                                        ->get<component::Controller>()
                                        .mType = ControllerType::Dummy;
                                    oldController->get(update)
                                        ->get<component::Controller>()
                                        .mControllerId = gDummyControllerIndex;
                                }
                                aPlayer.get(update)
                                    ->get<component::Controller>()
                                    .mType = ControllerType::Keyboard;
                                aPlayer.get(update)
                                    ->get<component::Controller>()
                                    .mControllerId = gKeyboardControllerIndex;
                            }

                            if (ImGui::Button("Remove player"))
                            {
                                removePlayerFromGame(update, aPlayer);
                            }
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                        ImGui::BeginChild("Info");
                        geo.drawUi();
                        pose.drawUi();
                        moveState.drawUi();
                        controller.drawUi();
                        ImGui::EndChild();
                    }
                    else
                    {
                        if (ImGui::Button("Create dummy player"))
                        {
                            {
                                fillSlotWithPlayer(
                                    mGameContext, ControllerType::Dummy,
                                    aPlayer, gDummyControllerIndex);
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
            snac::imguiDebugDrawerLevelSection(&mImguiDisplays.mShowDebugDrawers);
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
                    aSettings.mSimulationDelta)
                    .count();
            if (ImGui::InputInt("Simulation period (ms)", &simulationDelta))
            {
                simulationDelta = std::clamp(simulationDelta, 1, 200);
            }
            aSettings.mSimulationDelta =
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
            ImGui::Begin("Main profiler", &mImguiDisplays.mShowMainProfiler);
            std::string str;
            snac::getProfiler(snac::Profiler::Main).print(str);
            ImGui::TextUnformatted(str.c_str());
            ImGui::End();
        }
        if (mImguiDisplays.mShowRenderProfiler)
        {
            ImGui::Begin("Render profiler",
                         &mImguiDisplays.mShowRenderProfiler);
            ImGui::TextUnformatted(
                snac::getRenderProfilerPrint().get().c_str());
            ImGui::End();
        }
        if (mImguiDisplays.mShowRenderControls)
        {
            // TODO This should be an easier raii object
            ImGui::Begin("Render controls", &mImguiDisplays.mShowRenderControls);
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
                ent::Query<component::Pill>{mGameContext.mWorld}.each([&destroy](EntHandle aHandle, const component::Pill &){
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
                EntHandle player = *snac::getFirstHandle(
                    ent::Query<component::PlayerSlot, component::Geometry>{
                        mGameContext.mWorld});
                {
                    Phase pathfinding;
                    Entity pEntity = *pathfinder.get(pathfinding);
                    addMeshGeoNode(pathfinding, mGameContext, pEntity, "CUBE",
                                   {7.f, 7.f, gPillHeight}, 1.f,
                                   {0.5f, 0.5f, 0.5f});
                    pEntity.add(component::AllowedMovement{
                        .mWindow = gOtherTurningZoneHalfWidth});
                    pEntity.add(component::PathToOnGrid{player});
                }
                EntHandle root = mStateMachine->getCurrentScene()->mSceneRoot;
                Phase phase;
                EntHandle level =
                    *root.get(phase)->get<component::SceneNode>().mFirstChild;
                insertEntityInScene(pathfinder, level);
            }
            ImGui::End();
        }
    }

    if (recompileShaders)
    {
        mGameContext.mRenderThread.recompileShaders(mGameContext.mResources)
            .get(); // so any exception is rethrown in this context
    }
}

bool SnacGame::update(const snac::Time & aTime, RawInput & aInput)
{
    snac::DebugDrawer::StartFrame();

    mSystemOrbitalCamera->update(
        aInput,
        snac::Camera::gDefaults.vFov, // TODO Should be dynamic
        mAppInterface->getWindowSize().height());

    if (!mGameContext.mSimulationControl.mPlaying
        && !mGameContext.mSimulationControl.mStep)
    {
        return false;
    }

    //float updateDelta = aDelta / mGameContext.mSimulationControl.mSpeedRatio;
    snac::Clock::duration updateDelta = aTime.mSimulationDeltaDuration;

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    std::optional<scene::Transition> transition =
        mStateMachine->getCurrentScene()->update(aTime, aInput);

    if (transition)
    {
        if (transition.value().mTransitionName.compare(
                scene::gQuitTransitionName)
            == 0)
        {
            return true;
        }
        else
        {
            mStateMachine->changeState(mGameContext, transition.value(),
                                       aInput);
        }
    }

    if (mGameContext.mSimulationControl.mSaveGameState)
    {
        TIME_RECURRING(Main, "Save state");
        mGameContext.mSimulationControl.saveState(
            mGameContext.mWorld.saveState(),
            std::chrono::duration_cast<std::chrono::milliseconds>(aTime.mSimulationDeltaDuration),
            std::chrono::duration_cast<std::chrono::milliseconds>(updateDelta));
    }

    return false;
}

std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;

    //
    // Worldspace models
    //
    mQueryRenderable.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       const component::GlobalPose & aGlobPose,
                       // TODO #anim restore this constness (for the moment, animation mutate the rig's scene)
                       /*const*/ component::VisualModel & aVisualModel)
        {
            // Note: This feels bad to test component presence here
            // but we do not want to handle VisualModel the same way depending on the presence of RigAnimation
            // (and we do not have "negation" on Queries, to separately get VisualModels without RigAnimation)
            visu::Entity::SkeletalAnimation skeletal;
            if(auto entity = *aHandle.get(); entity.has<component::RigAnimation>())
            {
                const auto & rigAnimation = aHandle.get()->get<component::RigAnimation>();
                skeletal = visu::Entity::SkeletalAnimation{
                    .mRig = &aVisualModel.mModel->mRig,
                    .mAnimation = rigAnimation.mAnimation,
                    .mParameterValue = rigAnimation.mParameterValue,
                };
            }
            state->mEntities.insert(
                aHandle.id(),
                visu::Entity{
                    .mPosition_world = aGlobPose.mPosition,
                    .mScaling = aGlobPose.mInstanceScaling *
                                    aGlobPose.mScaling,
                    .mOrientation = aGlobPose.mOrientation,
                    .mColor = aGlobPose.mColor,
                    .mModel = aVisualModel.mModel,
                    .mRigging = std::move(skeletal),
                });
        });

    //
    // Worldspace Text
    //
    mQueryTextWorld.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       component::Text & aText,
                       component::GlobalPose & aGlobPose) 
        {
            state->mTextWorldEntities.insert(
                aHandle.id(),
                visu::Text{
                    .mPosition_world = aGlobPose.mPosition,
                    // TODO remove the hardcoded value of 100
                    // Note hardcoded 100 scale down. Because I'd like a value of 1 for the scale of the component
                    // to still mean "about visible".
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
                       component::PoseScreenSpace & aPose) 
        {

            math::Position<3, float> position_screenPix{
                aPose.mPosition_u.cwMul(
                    // TODO this multiplication should be done once and cached
                    // but it should be refreshed on framebuffer resizing.
                    static_cast<math::Position<2, GLfloat>>(this->mAppInterface->getFramebufferSize())/2.f),
                    0.f
            };

            state->mTextScreenEntities.insert(
                aHandle.id(),
                visu::Text{
                    .mPosition_world = position_screenPix,
                    .mScaling = math::Size<3, float>{aPose.mScale, 1.f}, 
                    .mOrientation = math::Quaternion{
                                                math::UnitVec<3, float>::MakeFromUnitLength({0.f, 0.f, 1.f}),
                                                aPose.mRotationCCW
                                            },
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
