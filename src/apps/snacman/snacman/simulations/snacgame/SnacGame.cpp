#include "SnacGame.h"

#include "Entities.h"
#include "InputCommandConverter.h"

#include "context/InputDeviceDirectory.h"

#include "component/Controller.h"
#include "component/Level.h"

#include "snacman/simulations/snacgame/component/Context.h"
#include "system/DeterminePlayerAction.h"
#include "system/IntegratePlayerMovement.h"
#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"

#include <markovjunior/Grid.h>
#include <math/Color.h>
#include <math/VectorUtilities.h>

#include <GLFW/glfw3.h>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface) :
    mAppInterface{&aAppInterface},
    mSystems{mWorld.addEntity()},
    mLevel{mWorld.addEntity()},
    mContext{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld}
{
    ent::Phase init;
    InputDeviceDirectory inputDeviceDirectory;
    markovjunior::Interpreter markovInterpreter(
        "/home/franz/gamedev/snac-assets", "markov/snaclvl.xml",
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

    markovjunior::Grid grid = markovInterpreter.mGrid;

    createLevel(mLevel, mWorld, init, grid);
    createPlayerEntity(
        mWorld, init,
        inputDeviceDirectory,
        component::ControllerType::Keyboard);

    mContext.get(init)->add(component::Context{inputDeviceDirectory});
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
                GamepadMapping({{GamepadAtomicInput::leftXAxisNegative,
                                 gPlayerMoveFlagLeft},
                                {GamepadAtomicInput::leftXAxisPositive,
                                 gPlayerMoveFlagRight},
                                {GamepadAtomicInput::leftYAxisNegative,
                                 gPlayerMoveFlagDown},
                                {GamepadAtomicInput::leftYAxisPositive,
                                 gPlayerMoveFlagUp}}));
        }
    }

    auto & keyboardPlayer = directory.mPlayerBoundToKeyboard;

    if (keyboardPlayer && keyboardPlayer->isValid())
    {
            component::Controller & keyboard =
                keyboardPlayer->get(update)
                    ->get<component::Controller>();

            keyboard.mCommandQuery = convertKeyboardInput(
                aInput.mKeyboard,
                KeyboardMapping({{GLFW_KEY_LEFT,
                                 gPlayerMoveFlagLeft},
                                {GLFW_KEY_RIGHT,
                                 gPlayerMoveFlagRight},
                                {GLFW_KEY_DOWN,
                                 gPlayerMoveFlagDown},
                                {GLFW_KEY_UP,
                                 gPlayerMoveFlagUp},
                                {GLFW_KEY_ESCAPE,
                                 gQuitCommand}}));

            quitGame = keyboard.mCommandQuery & gQuitCommand;
    }

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput, getCameraParameters().vFov,
                                 mAppInterface->getWindowSize().height());
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);
    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::DeterminePlayerAction>().update();
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);

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
            }
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
