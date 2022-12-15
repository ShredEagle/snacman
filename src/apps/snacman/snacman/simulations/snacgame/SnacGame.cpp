#include "SnacGame.h"

#include "ActionKeyMapper.h"
#include "component/Level.h"
#include "Entities.h"
#include "markovjunior/Grid.h"
#include "math/Color.h"
#include "system/DeterminePlayerAction.h"
#include "system/IntegratePlayerMovement.h"
#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"

#include <math/VectorUtilities.h>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface) :
    mAppInterface{&aAppInterface},
    mSystems{mWorld.addEntity()},
    mLevel{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld}
{
    ent::Phase init;
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
    createPlayerEntity(mWorld, init);
}

void SnacGame::update(float aDelta, const snac::Input & aInput)
{
    mSimulationTime += aDelta;

    ent::Phase update;

    // mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput, getCameraParameters().vFov,
                                 mAppInterface->getWindowSize().height());
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);
    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::DeterminePlayerAction>().update(aInput);
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);
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
                        + (float) aGeometry.mGridPosition.y() * cellSize + aGeometry.mSubGridPosition.y(),
                    yCoord,
                    -(float) mColCount
                        + (float) aGeometry.mGridPosition.x() * cellSize + aGeometry.mSubGridPosition.x(),
                };
                state->mEntities.insert(aHandle.id(),
                                        visu::Entity{
                                            .mPosition_world = worldPosition,
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
