#include "SnacGame.h"

#include "Entities.h"

#include "system/PlayerInvulFrame.h"
#include "system/PlayerSpawner.h"

#include "markovjunior/Grid.h"
#include "math/Color.h"
#include <math/VectorUtilities.h>

namespace ad {
namespace snacgame {

SnacGame::SnacGame(graphics::AppInterface & aAppInterface) :
    mAppInterface{&aAppInterface},
    mSystems{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld}
{
    ent::Phase init;
    markovjunior::Interpreter markovInterpreter(
        "/home/franz/gamedev/snac-assets", "markov/snaclvl.xml",
        ad::math::Size<3, int>{29, 29, 1}, 1231234);

    markovInterpreter.setup();

    while (markovInterpreter.mCurrentBranch != nullptr) {
        markovInterpreter.runStep();
    }
    mSystems.get(init)->add(system::PlayerSpawner{mWorld});
    mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});

    markovjunior::Grid grid = markovInterpreter.mGrid;

    for (int z = 0; z < grid.mSize.depth(); z++) {
        for (int y = 0; y < grid.mSize.height(); y++) {
            for (int x = 0; x < grid.mSize.width(); x++) {
                unsigned char value =
                    grid.mCharacters.at(grid.mState.at(grid.getFlatGridIndex({x, y, z})));
                switch (value) {
                case 'W':
                    createPathEntity(mWorld, init,
                                     math::Position<3, float>{
                                         -grid.mSize.height() + (float)y * 2.f, 0.f,
                                         grid.mSize.width() - (float)x * 2.f});
                    break;
                case 'K':
                    createPortalEntity(mWorld, init,
                                       math::Position<3, float>{
                                           -grid.mSize.height() + (float)y * 2.f, 0.f,
                                           grid.mSize.width() - (float)x * 2.f});
                case 'O':
                    createCopPenEntity(mWorld, init,
                                       math::Position<3, float>{
                                           -grid.mSize.height() + (float)y * 2.f, 0.f,
                                           grid.mSize.width() - (float)x * 2.f});
                    break;
                case 'U':
                    createPlayerSpawnEntity(
                        mWorld, init,
                        math::Position<3, float>{-grid.mSize.height() + (float)y * 2.f,
                                                 0.f,
                                                 grid.mSize.width() - (float)x * 2.f});
                    break;
                default:
                    break;
                }
            }
        }
    }

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
}

std::unique_ptr<visu::GraphicState> SnacGame::makeGraphicState() const
{
    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;
    mQueryRenderable.get(nomutation)
        .each(
            [&state](ent::Handle<ent::Entity> aHandle, component::Geometry & aGeometry) {
                if (aGeometry.mShouldBeDrawn) {
                    state->mEntities.insert(aHandle.id(),
                                            visu::Entity{
                                                .mPosition_world = aGeometry.mPosition,
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
