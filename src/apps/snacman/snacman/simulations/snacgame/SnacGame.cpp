#include "SnacGame.h"

#include "Entities.h"
#include "InputCommandConverter.h"

#include "context/InputDeviceDirectory.h"

#include "component/Context.h"
#include "component/Controller.h"
#include "component/Level.h"
#include "component/MovementScreenSpace.h"


#include "system/DeterminePlayerAction.h"
#include "system/IntegratePlayerMovement.h"
#include "system/MovementIntegration.h"
#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"

#include <snacman/ImguiUtilities.h>

#include <imguiui/ImguiUi.h>

#include <snac-renderer/text/Text.h>

#include <imgui.h>
#include <markovjunior/Grid.h>

#include <math/Color.h>
#include <math/VectorUtilities.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface,
                   imguiui::ImguiUi & aImguiUi,
                   const resource::ResourceFinder & aResourceFinder) :
    mAppInterface{&aAppInterface},
    mSystems{mWorld.addEntity()},
    mLevel{mWorld.addEntity()},
    mContext{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld},
    mQueryText{mWorld, mWorld},
    mImguiUi{aImguiUi}
{
    ent::Phase init;
    InputDeviceDirectory inputDeviceDirectory;
    markovjunior::Interpreter markovInterpreter(
        // Address this instead of comment-fighting : )
        //"/home/franz/gamedev/snac-assets", "markov/snaclvl.xml",
        "d:/projects/gamedev/2/snac-assets", "markov/snaclvl.xml",
        ad::math::Size<3, int>{29, 29, 1}, 1231234);

    markovInterpreter.setup();

    while (markovInterpreter.mCurrentBranch != nullptr)
    {
        markovInterpreter.runStep();
    }
    mSystems.get(init)->add(system::PlayerSpawner{mWorld});
    mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});
    mSystems.get(init)->add(system::DeterminePlayerAction{mWorld, mLevel});
    mSystems.get(init)->add(system::IntegratePlayerMovement{mWorld, mLevel});
    mSystems.get(init)->add(system::MovementIntegration{mWorld});

    markovjunior::Grid grid = markovInterpreter.mGrid;

    createLevel(mLevel, mWorld, init, grid);
    createPlayerEntity(mWorld, init, inputDeviceDirectory,
                       component::ControllerType::Keyboard);

    ent::Handle<ent::Entity> title = 
        makeText(mWorld,
                init,
                "Snacman!",
                // TODO this should be done within the ResourceManager, here only fetching the Font via name + size
                std::make_shared<snac::Font>(
                    mFreetype,
                    *aResourceFinder.find("fonts/Comfortaa-Regular.ttf"),
                    120,
                    snac::makeDefaultTextProgram(aResourceFinder)
                ),
                math::hdr::gYellow<float>,
                math::Position<2, float>{-0.5f, 0.f});
    // TODO Remove, this is a silly demonstration.
    title.get(init)->add(component::MovementScreenSpace{
        .mAngularSpeed = math::Radian<float>{math::pi<float> / 2.f}
    });

    mContext.get(init)->add(component::Context(inputDeviceDirectory, aResourceFinder));
}

void SnacGame::drawDebugUi(snac::ConfigurableSettings & aSettings,
                           ImguiInhibiter & aInhibiter,
                           const RawInput & aInput)
{
    ent::Phase update;

    component::Context & context =
        mContext.get(update)->get<component::Context>();

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
        context.drawUi(aInput);
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
    bool quitGame = false;

    mSimulationTime += aDelta;

    ent::Phase update;

    component::Context & context =
        mContext.get(update)->get<component::Context>();
    InputDeviceDirectory & directory = context.mInputDeviceDirectory;

    for (auto & connectedPlayer : directory.mGamepadBindings)
    {
        if (connectedPlayer.mPlayer && connectedPlayer.mPlayer->isValid())
        {
            component::Controller & controller =
                connectedPlayer.mPlayer->get(update)
                    ->get<component::Controller>();
            controller.mCommandQuery = convertGamepadInput(
                aInput.mGamepads.at(connectedPlayer.mRawInputGamepadIndex),
                GamepadMapping(context.mGamepadMapping));
        }
    }

    auto & keyboardPlayer = directory.mPlayerBoundToKeyboard;

    if (keyboardPlayer && keyboardPlayer->isValid())
    {
        component::Controller & keyboard =
            keyboardPlayer->get(update)->get<component::Controller>();

        keyboard.mCommandQuery = convertKeyboardInput(
            aInput.mKeyboard, KeyboardMapping(context.mKeyboardMapping));

        quitGame = keyboard.mCommandQuery & gQuitCommand;
    }

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput, getCameraParameters().vFov,
                                 mAppInterface->getWindowSize().height());
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);
    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::DeterminePlayerAction>().update();
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);
    mSystems.get(update)->get<system::MovementIntegration>().update(aDelta);

    return quitGame;
}

std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState()
{
    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;
    ent::Entity levelEntity = *mLevel.get(nomutation);
    const float cellSize = levelEntity.get<component::Level>().mCellSize;
    const int mRowCount = levelEntity.get<component::Level>().mRowCount;
    const int mColCount = levelEntity.get<component::Level>().mColCount;
    mQueryRenderable.get(nomutation)
        .each([mRowCount, mColCount, cellSize, &state](
                  ent::Handle<ent::Entity> aHandle,
                  component::Geometry & aGeometry) {
            if (aGeometry.mShouldBeDrawn)
            {
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
                                            .mYAngle = aGeometry.mYRotation,
                                            .mColor = aGeometry.mColor,
                                        });
                std::function<void()> func = [worldPosition]() {
                    float stuff = worldPosition.x();
                    ImGui::InputFloat("Stuff:", &stuff);
                };
                state->mImguiCommands.push_back(func);
            }
        });

    //
    // Text
    //
    mQueryText.get(nomutation).each(
        [&state](ent::Handle<ent::Entity> aHandle, component::Text & aText, component::PoseScreenSpace & aPose)
        {
            state->mTextEntities.insert(
                aHandle.id(),
                visu::TextScreen{
                    // TODO
                    .mPosition_unitscreen = aPose.mPosition_u, 
                    .mOrientation = aPose.mRotationCCW,
                    .mString = aText.mString,
                    .mFont = aText.mFont,
                    .mColor = aText.mColor,
            });
        }
    );


    state->mCamera = mSystemOrbitalCamera->getCamera();
    return state;
}

snac::Camera::Parameters SnacGame::getCameraParameters() const
{
    return mCameraParameters;
}

SnacGame::Renderer_t SnacGame::makeRenderer(const resource::ResourceFinder & aResourceFinder) const
{
    return Renderer_t{
        *mAppInterface,
        getCameraParameters(),
        aResourceFinder
    };
}

} // namespace snacgame
} // namespace ad
