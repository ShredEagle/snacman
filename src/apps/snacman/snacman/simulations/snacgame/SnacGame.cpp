#include "SnacGame.h"

#include "Entities.h"
#include "GameContext.h"
#include "InputCommandConverter.h"

#include "component/MovementScreenSpace.h"

#include "component/Context.h"
#include "component/Controller.h"
#include "component/LevelData.h"
#include "component/MovementScreenSpace.h"
#include "component/PlayerSlot.h"
#include "Entities.h"
#include "InputCommandConverter.h"
#include "scene/Scene.h"
#include "snacman/ImguiUtilities.h"
#include "snacman/simulations/snacgame/component/PlayerSlot.h"
#include "system/DeterminePlayerAction.h"
#include "system/IntegratePlayerMovement.h"
#include "system/MovementIntegration.h"
#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"
#include "system/SceneStateMachine.h"

#include <snacman/ImguiUtilities.h>
#include <snacman/Profiling.h>

#include <imguiui/ImguiUi.h>

#include <snac-renderer/text/Text.h>

#include <imgui.h>
#include <imguiui/ImguiUi.h>
#include <markovjunior/Grid.h>
#include <math/Color.h>
#include <math/VectorUtilities.h>
#include <optional>
#include <snac-renderer/text/Text.h>
#include <string>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface,
                   snac::RenderThread<Renderer_t> & aRenderThread,
                   imguiui::ImguiUi & aImguiUi,
                   resource::ResourceFinder aResourceFinder,
                   RawInput & aInput) :
    mAppInterface{&aAppInterface},
    mGameContext{
        .mResources = snac::Resources{std::move(aResourceFinder), aRenderThread},
        .mRenderThread = aRenderThread,
    },
    mMappingContext{mGameContext.mWorld, mGameContext.mResources},
    mStateMachine{mGameContext.mWorld,
                  mGameContext.mWorld,
                  *mGameContext.mResources.find("scenes/scene_description.json"),
                  mMappingContext},
    mSystemOrbitalCamera{mGameContext.mWorld, mGameContext.mWorld},
    mQueryRenderable{mGameContext.mWorld, mGameContext.mWorld},
    mQueryText{mGameContext.mWorld, mGameContext.mWorld},
    mImguiUi{aImguiUi}
{
    ent::Phase init;

    // Creating the slot entity those will be used a player entities
    mGameContext.mWorld.addEntity().get(init)->add(component::PlayerSlot{0, false});
    mGameContext.mWorld.addEntity().get(init)->add(component::PlayerSlot{1, false});
    mGameContext.mWorld.addEntity().get(init)->add(component::PlayerSlot{2, false});
    mGameContext.mWorld.addEntity().get(init)->add(component::PlayerSlot{3, false});

    scene::Scene * scene = mStateMachine->getCurrentScene();
    scene->setup(mGameContext, scene::Transition{}, aInput);
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           RawInput & aInput)
{
    TIME_RECURRING_FUNC(Main);

    ent::Phase update;

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

    mImguiDisplays.display();
    if (mImguiDisplays.mShowLogLevel)
    {
        snac::imguiLogLevelSelection();
    }
    if (mImguiDisplays.mShowMappings)
    {
        mMappingContext->drawUi(aInput);
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
    if (mImguiDisplays.mShowMainProfiler)
    {
        ImGui::Begin("Main profiler");
        std::string str;
        snac::getProfiler(snac::Profiler::Main).print(str);
        ImGui::TextUnformatted(str.c_str());
        ImGui::End();
    }
    if (mImguiDisplays.mShowRenderProfiler)
    {
        ImGui::Begin("Render profiler");
        ImGui::TextUnformatted(snac::getRenderProfilerPrint().get().c_str());
        ImGui::End();
    }

    mImguiUi.render();
    mImguiUi.mFrameMutex.unlock();
}


bool SnacGame::update(float aDelta, RawInput & aInput)
{
    mSimulationTime += aDelta;

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput,
                                 snac::Camera::gDefaults.vFov, // TODO Should be dynamic
                                 mAppInterface->getWindowSize().height());

    std::optional<scene::Transition> transition =
        mStateMachine->getCurrentScene()->update(mGameContext, aDelta, aInput);

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
            mStateMachine->changeState(mGameContext, transition.value(), aInput);
        }
    }

    return false;
}


std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;
    const float cellSize = gCellSize;
    const int mRowCount = gGridSize;
    const int mColCount = gGridSize;

    mQueryRenderable.get(nomutation)
        .each([cellSize, &state](
                  ent::Handle<ent::Entity> aHandle,
                  const component::Geometry & aGeometry,
                  const component::VisualMesh & aVisualMesh) {
            float yCoord =
                aGeometry.mLayer == component::GeometryLayer::Level
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
                                        .mOrientation = aGeometry.mOrientation,
                                        .mColor = aGeometry.mColor,
                                        .mMesh = aVisualMesh.mMesh,
                                    });
            std::function<void()> func = [worldPosition]() {
                float stuff = worldPosition.x();
                ImGui::InputFloat("Stuff:", &stuff);
            };
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
