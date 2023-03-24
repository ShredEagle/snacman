#include "SnacGame.h"

#include "component/Context.h"
#include "component/Geometry.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/Text.h"
#include "component/VisualMesh.h"
#include "GameContext.h"
#include "GameParameters.h"
#include "InputConstants.h"
#include "scene/Scene.h"
#include "SimulationControl.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "system/SceneStateMachine.h"
#include "system/SystemOrbitalCamera.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <imgui.h>
#include <imguiui/ImguiUi.h>
#include <map>
#include <math/Color.h>
#include <mutex>
#include <optional>
#include <snacman/ImguiUtilities.h>
#include <snacman/LoopSettings.h>
#include <snacman/Profiling.h>
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
                   RawInput & aInput) :
    mAppInterface{&aAppInterface},
    mGameContext{
        .mResources =
            snac::Resources{std::move(aResourceFinder), aRenderThread},
        .mRenderThread = aRenderThread,
    },
    mMappingContext{mGameContext.mWorld, mGameContext.mResources},
    mStateMachine{
        mGameContext.mWorld, mGameContext.mWorld,
        *mGameContext.mResources.find("scenes/scene_description.json"),
        mMappingContext, mGameContext},
    mSystemOrbitalCamera{mGameContext.mWorld, mGameContext.mWorld},
    mQueryRenderable{mGameContext.mWorld, mGameContext.mWorld},
    mQueryText{mGameContext.mWorld, mGameContext.mWorld},
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
    scene->setup(mGameContext, scene::Transition{}, aInput);
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           RawInput & aInput)
{
    TIME_RECURRING_FUNC(Main);

    mImguiUi.mFrameMutex.lock();
    {
        TIME_RECURRING(Main, "new frame");
        mImguiUi.newFrame();
    }
    // NewFrame() updates the io catpure flag: consume them ASAP
    // see: https://pixtur.github.io/mkdocs-for-imgui/site/FAQ/#qa-integration
    // mImguiUi.newFrame();
    aInhibiter.resetCapture(static_cast<ImguiInhibiter::WantCapture>(
        (mImguiUi.isCapturingMouse() ? ImguiInhibiter::Mouse
                                     : ImguiInhibiter::Null)
        | (mImguiUi.isCapturingKeyboard() ? ImguiInhibiter::Keyboard
                                          : ImguiInhibiter::Null)));

    if (mImguiDisplays.mShowPlayerInfo)
    {
        ent::Phase update;
        ent::Query<component::PlayerSlot, component::Geometry,
                   component::PlayerMoveState, component::GlobalPose>
            playerQuery{mGameContext.mWorld};
        int playerIndex = 0;
        ImGui::Begin("Player Info", &mImguiDisplays.mShowPlayerInfo);
        playerQuery.each([&](const component::Geometry & aPlayerGeometry,
                             const component::PlayerMoveState & aMoveState, const component::GlobalPose & aPose) {
            int intPosX =
                static_cast<int>(aPlayerGeometry.mPosition.x() + 0.5f);
            int intPosY =
                static_cast<int>(aPlayerGeometry.mPosition.y() + 0.5f);
            float fracPosX = aPlayerGeometry.mPosition.x() - intPosX;
            float fracPosY = aPlayerGeometry.mPosition.y() - intPosY;

            char playerHeader[64];
            std::snprintf(playerHeader, IM_ARRAYSIZE(playerHeader), "Player %d",
                          playerIndex);
            if (ImGui::CollapsingHeader(playerHeader))
            {
                ImGui::Text("Player pos: %f, %f %f", aPlayerGeometry.mPosition.x(),
                            aPlayerGeometry.mPosition.y(), aPlayerGeometry.mPosition.z());
                ImGui::Text("Player integral part: %d, %d", intPosX, intPosY);
                ImGui::Text("Player frac part: %f, %f", fracPosX, fracPosY);
                ImGui::Text("Player orientation: %f, (%f, %f, %f)", aPlayerGeometry.mOrientation.w(),
                            aPlayerGeometry.mOrientation.x(), aPlayerGeometry.mOrientation.y(), aPlayerGeometry.mOrientation.z());
                ImGui::Text("Player instance scaling: %f, %f %f", aPlayerGeometry.mInstanceScaling.width(),
                            aPlayerGeometry.mInstanceScaling.height(), aPlayerGeometry.mInstanceScaling.depth());
                ImGui::Text("Player global pos: %f, %f, %f", aPose.mPosition.x(),
                            aPose.mPosition.y(), aPose.mPosition.z());
                ImGui::Text("Player global scaling: %f", aPose.mScaling);
                ImGui::Text("Player global orientation: %f, (%f, %f, %f)", aPose.mOrientation.w(),
                            aPose.mOrientation.x(), aPose.mOrientation.y(), aPose.mOrientation.z());
                ImGui::Text("Player global instance scaling: %f, %f %f", aPose.mInstanceScaling.width(),
                            aPose.mInstanceScaling.width(), aPose.mInstanceScaling.depth());
                ImGui::Text("Player MoveState:");
                if (aMoveState.mAllowedMove & gPlayerMoveFlagDown)
                {
                    ImGui::SameLine();
                    ImGui::Text("Down");
                }
                if (aMoveState.mAllowedMove & gPlayerMoveFlagUp)
                {
                    ImGui::SameLine();
                    ImGui::Text("Up");
                }
                if (aMoveState.mAllowedMove & gPlayerMoveFlagRight)
                {
                    ImGui::SameLine();
                    ImGui::Text("Right");
                }
                if (aMoveState.mAllowedMove & gPlayerMoveFlagLeft)
                {
                    ImGui::SameLine();
                    ImGui::Text("Left");
                }
            }
        });
        ImGui::End();
    }

    mImguiDisplays.display();
    if (mImguiDisplays.mShowLogLevel)
    {
        snac::imguiLogLevelSelection(&mImguiDisplays.mShowLogLevel);
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
        int simulationDelta = (int) duration_cast<std::chrono::milliseconds>(
                                  aSettings.mSimulationDelta)
                                  .count();
        if (ImGui::InputInt("Simulation period (ms)", &simulationDelta))
        {
            simulationDelta = std::clamp(simulationDelta, 1, 200);
        }
        aSettings.mSimulationDelta = std::chrono::milliseconds{simulationDelta};

        int updateDuration = (int) duration_cast<std::chrono::milliseconds>(
                                 aSettings.mUpdateDuration)
                                 .count();
        if (ImGui::InputInt("Update duration (ms)", &updateDuration))
        {
            updateDuration = std::clamp(updateDuration, 1, 500);
        }
        aSettings.mUpdateDuration = std::chrono::milliseconds{updateDuration};

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
        ImGui::Begin("Render profiler", &mImguiDisplays.mShowRenderProfiler);
        ImGui::TextUnformatted(snac::getRenderProfilerPrint().get().c_str());
        ImGui::End();
    }

    mImguiUi.render();
    mImguiUi.mFrameMutex.unlock();
}

bool SnacGame::update(float aDelta, RawInput & aInput)
{
    mSystemOrbitalCamera->update(
        aInput,
        snac::Camera::gDefaults.vFov, // TODO Should be dynamic
        mAppInterface->getWindowSize().height());

    if (!mGameContext.mSimulationControl.mPlaying
        && !mGameContext.mSimulationControl.mStep)
    {
        return false;
    }

    float updateDelta = aDelta / mGameContext.mSimulationControl.mSpeedRatio;
    mSimulationTime += aDelta;

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    std::optional<scene::Transition> transition =
        mStateMachine->getCurrentScene()->update(mGameContext, updateDelta,
                                                 aInput);

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
            std::chrono::microseconds{static_cast<long>(aDelta * 1'000'000)},
            std::chrono::microseconds{
                static_cast<long>(updateDelta * 1'000'000)});
    }

    return false;
}

std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;

    mQueryRenderable.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       const component::GlobalPose & aGlobPose,
                       const component::VisualMesh & aVisualMesh) {
            state->mEntities.insert(
                aHandle.id(),
                visu::Entity{
                    .mPosition_world = aGlobPose.mPosition,
                    .mScaling = math::Size<3, float>{aGlobPose.mScaling,
                                                     aGlobPose.mScaling,
                                                     aGlobPose.mScaling}
                                    .cwMul(aGlobPose.mInstanceScaling),
                    .mOrientation = aGlobPose.mOrientation,
                    .mColor = aGlobPose.mColor,
                    .mMesh = aVisualMesh.mMesh,
                });
        });

    //
    // Text
    //
    mQueryText.get(nomutation)
        .each([&state](ent::Handle<ent::Entity> aHandle,
                       component::Text & aText,
                       component::PoseScreenSpace & aPose) {
            state->mTextEntities.insert(
                aHandle.id(), visu::TextScreen{
                                  // TODO
                                  .mPosition_unitscreen = aPose.mPosition_u,
                                  .mScale = aPose.mScale,
                                  .mOrientation = aPose.mRotationCCW,
                                  .mString = aText.mString,
                                  .mFont = aText.mFont,
                                  .mColor = aText.mColor,
                              });
        });

    state->mCamera = mSystemOrbitalCamera->getCamera();
    return state;
}

} // namespace snacgame
} // namespace ad
