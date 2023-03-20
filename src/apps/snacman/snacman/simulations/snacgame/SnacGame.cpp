#include "SnacGame.h"

#include "component/Context.h"
#include "GameContext.h"
#include "GameParameters.h"
#include "InputConstants.h"
#include "scene/Scene.h"
#include "SimulationControl.h"
#include "snacman/ImguiUtilities.h"
#include "snacman/LoopSettings.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/PlayerMoveState.h"
#include "snacman/simulations/snacgame/component/PlayerSlot.h"
#include "system/SceneStateMachine.h"
#include "system/SystemOrbitalCamera.h"
#include "component/VisualMesh.h"
#include "component/Text.h"
#include "component/PoseScreenSpace.h"

#include <snacman/Profiling.h>

#include <handy/Guard.h>

#include <imguiui/ImguiUi.h>
#include <math/Color.h>

#include <imgui.h>

#include <string>
#include <optional>
#include <algorithm>
#include <cstdio>
#include <array>
#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>

namespace ad {
struct RawInput;

namespace snac { template <class T_renderer> class RenderThread; }
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
        .mResources = 
            snac::Resources{std::move(aResourceFinder), aFreetype, aRenderThread},
        .mRenderThread = aRenderThread,
    },
    mMappingContext{mGameContext.mWorld, mGameContext.mResources},
    mStateMachine{
        mGameContext.mWorld, mGameContext.mWorld,
        *mGameContext.mResources.find("scenes/scene_description.json"),
        mMappingContext},
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

    bool recompileShaders = false;
    {
        std::lock_guard lock{mImguiUi.mFrameMutex};
        mImguiUi.newFrame();
        // We must make sure to match each newFrame() to a render(),
        // otherwise we might get access violation in the RenderThread, when calling ImguiUi::renderBackend()
        Guard renderGuard{[this]()
        {
            this->mImguiUi.render();
        }};

        // NewFrame() updates the io catpure flag: consume them ASAP
        // see: https://pixtur.github.io/mkdocs-for-imgui/site/FAQ/#qa-integration
        aInhibiter.resetCapture(static_cast<ImguiInhibiter::WantCapture>(
            (mImguiUi.isCapturingMouse() ? ImguiInhibiter::Mouse
                                        : ImguiInhibiter::Null)
            | (mImguiUi.isCapturingKeyboard() ? ImguiInhibiter::Keyboard
                                            : ImguiInhibiter::Null)));

        mImguiDisplays.display();

        if (mImguiDisplays.mShowPlayerInfo)
        {
            ent::Phase update;
            ent::Query<component::PlayerSlot, component::Geometry, component::PlayerMoveState> playerQuery{
                mGameContext.mWorld};
            int playerIndex = 0;
            ImGui::Begin("Player Info", &mImguiDisplays.mShowPlayerInfo);
            playerQuery.each([&](const component::Geometry & aPlayerGeometry, component::PlayerMoveState & aMoveState) {
                int intPosX =
                    static_cast<int>(aPlayerGeometry.mPosition.x() + 0.5f);
                int intPosY =
                    static_cast<int>(aPlayerGeometry.mPosition.y() + 0.5f);
                float fracPosX = aPlayerGeometry.mPosition.x() - intPosX;
                float fracPosY = aPlayerGeometry.mPosition.y() - intPosY;

                char playerHeader[64];
                std::snprintf(playerHeader, IM_ARRAYSIZE(playerHeader), "Player %d", playerIndex);
                if(ImGui::CollapsingHeader(playerHeader))
                {
                    ImGui::Text("Player pos: %f, %f", aPlayerGeometry.mPosition.x(),
                                aPlayerGeometry.mPosition.y());
                    ImGui::Text("Player integral part: %d, %d", intPosX, intPosY);
                    ImGui::Text("Player frac part: %f, %f", fracPosX, fracPosY);
                    ImGui::Text("Current portal %d", aMoveState.mCurrentPortal);
                    ImGui::Text("Dest portal %d", aMoveState.mDestinationPortal);
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
            mGameContext.mSimulationControl.drawSimulationUi(mGameContext.mWorld, &mImguiDisplays.mSpeedControl);
        }
        if (mImguiDisplays.mShowSimulationDelta)
        {
            ImGui::Begin("Gameloop");
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
        if (mImguiDisplays.mShowRenderControls)
        {
            // TODO This should be an easier raii object
            ImGui::Begin("Render controls");
            Guard endGuard{[](){
                ImGui::End();
            }};
            recompileShaders = ImGui::Button("Recompile shaders");
        }
    }

    if (recompileShaders)
    {
        mGameContext.mRenderThread.recompileShaders(mGameContext.mResources)
            .get(); // so any exception is rethrown in this context
    }
}

bool SnacGame::update(float aDelta, RawInput & aInput)
{
    mSystemOrbitalCamera->update(
        aInput,
        snac::Camera::gDefaults.vFov, // TODO Should be dynamic
        mAppInterface->getWindowSize().height());

    if (!mGameContext.mSimulationControl.mPlaying && !mGameContext.mSimulationControl.mStep)
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
    const float cellSize = gCellSize;

    mQueryRenderable.get(nomutation)
        .each([cellSize, &state](ent::Handle<ent::Entity> aHandle,
                                 const component::Geometry & aGeometry,
                                 const component::VisualMesh & aVisualMesh) {
            float yCoord = static_cast<float>((int)aGeometry.mLayer) * cellSize * 0.1f;
            auto worldPosition = math::Position<3, float>{
                -(float) aGeometry.mPosition.y(),
                yCoord,
                -(float) aGeometry.mPosition.x(),
            };
            state->mEntities.insert(aHandle.id(),
                                    visu::Entity{
                                        .mPosition_world = worldPosition,
                                        .mScaling = aGeometry.mScaling,
                                        .mOrientation = aGeometry.mOrientation,
                                        .mColor = aGeometry.mColor,
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
