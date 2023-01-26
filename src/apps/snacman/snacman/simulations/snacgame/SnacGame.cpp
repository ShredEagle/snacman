#include "SnacGame.h"

#include "component/Context.h"
#include "component/Controller.h"
#include "component/LevelData.h"
#include "context/InputDeviceDirectory.h"
#include "Entities.h"
#include "imguiui/ImguiUi.h"
#include "InputCommandConverter.h"
#include "snacman/ImguiUtilities.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "system/DeterminePlayerAction.h"
#include "system/IntegratePlayerMovement.h"
#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"
#include "system/SceneStateMachine.h"

#include <chrono>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <markovjunior/Grid.h>
#include <math/Color.h>
#include <math/VectorUtilities.h>
#include <optional>
#include <string>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface,
                   imguiui::ImguiUi & aImguiUi) :
    mAppInterface{&aAppInterface},
    mContext{mWorld, InputDeviceDirectory{}},
    mStateMachine{mWorld, mWorld, gAssetRoot / "scenes/scene_description.json",
                  mContext},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld},
    mImguiUi{aImguiUi}
{
    ent::Phase init;

    /* createPlayerEntity(mWorld, init, inputDeviceDirectory, */
    /*                    component::ControllerType::Keyboard); */

    scene::Scene * scene = mStateMachine->getCurrentScene();
    scene->setup(scene::Transition{});
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           const RawInput & aInput)
{
    ent::Phase update;

    mImguiUi.mFrameMutex.lock();
    mImguiUi.newFrame();
    // NewFrame() updates the io catpure flag: consume them ASAP
    // see: https://pixtur.github.io/mkdocs-for-imgui/site/FAQ/#qa-integration
    // mImguiUi.newFrame();
    aInhibiter.resetCapture(static_cast<ImguiInhibiter::WantCapture>(
        (mImguiUi.isCapturingMouse() ? ImguiInhibiter::Mouse
                                     : ImguiInhibiter::Null)
        | (mImguiUi.isCapturingKeyboard() ? ImguiInhibiter::Keyboard
                                          : ImguiInhibiter::Null)));

    mImguiDisplays.display();
    if (mImguiDisplays.mShowLogLevel)
    {
        snac::imguiLogLevelSelection();
    }
    if (mImguiDisplays.mShowMappings)
    {
        mContext->drawUi(aInput);
    }
    if (mImguiDisplays.mShowImguiDemo)
    {
        ImGui::ShowDemoWindow();
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
    mImguiUi.render();
    mImguiUi.mFrameMutex.unlock();
}

bool SnacGame::update(float aDelta, const RawInput & aInput)
{
    mSimulationTime += aDelta;

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput, getCameraParameters().vFov,
                                 mAppInterface->getWindowSize().height());

    std::optional<scene::Transition> transition =
        mStateMachine->getCurrentScene()->update(aDelta, aInput);

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
            mStateMachine->changeState(transition.value());
        }
    }

    return false;
}

std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState()
{
    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;
    const float cellSize = gCellSize;
    const int mRowCount = gGridSize;
    const int mColCount = gGridSize;

    mQueryRenderable.get(nomutation)
        .each([cellSize, &state](ent::Handle<ent::Entity> aHandle,
                                 component::Geometry & aGeometry) {
            float yCoord = aGeometry.mLayer == component::GeometryLayer::Level
                               ? 0.f
                               : cellSize;
            auto worldPosition = math::Position<3, float>{
                -(float) mRowCount
                    + (float) aGeometry.mGridPosition.y() * cellSize
                    + aGeometry.mSubGridPosition.y(),
                yCoord,
                -(float) mColCount
                    + (float) aGeometry.mGridPosition.x() * cellSize
                    + aGeometry.mSubGridPosition.x(),
            };
            state->mEntities.insert(aHandle.id(),
                                    visu::Entity{
                                        .mPosition_world = worldPosition,
                                        .mScaling = aGeometry.mScaling,
                                        .mYAngle = aGeometry.mYRotation,
                                        .mColor = aGeometry.mColor,
                                    });
            std::function<void()> func = [worldPosition]() {
                float stuff = worldPosition.x();
                ImGui::InputFloat("Stuff:", &stuff);
            };
        });

    state->mCamera = mSystemOrbitalCamera->getCamera();
    return state;
}

snac::Camera::Parameters SnacGame::getCameraParameters() const
{
    return mCameraParameters;
}

SnacGame::Renderer_t SnacGame::makeRenderer() const
{
    return Renderer_t{
        math::getRatio<float>(mAppInterface->getWindowSize()),
        getCameraParameters(),
    };
}

} // namespace snacgame
} // namespace ad
