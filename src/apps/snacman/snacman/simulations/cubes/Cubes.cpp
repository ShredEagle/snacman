#include "Cubes.h"

#include "SystemMove.h"

#include <math/VectorUtilities.h>


namespace ad {
namespace cubes {


Cubes::Cubes(graphics::AppInterface & aAppInterface) :
    mAppInterface{&aAppInterface},
    mSystemMove{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld},
    mBlinkingCube{mWorld.addEntity()}
{
    ent::Phase init;
    mSystemMove.get(init)->add(system::Move{mWorld});

    // Add cubes
    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = { 1.5f, 0.f, 0.f},
        .mYRotation = math::Degree<float>{0.f},
    });

    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = {-1.5f, 0.f, 0.f},
        .mYRotation = math::Degree<float>{0.f},
    });
}


void Cubes::update(float aDelta, const snac::Input & aInput)
{
    mSimulationTime += aDelta; 

    ent::Phase update;

    blinkCube(update);

    mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput,
                                 getCameraParameters().vFov,
                                 mAppInterface->getWindowSize().height());
}

void Cubes::blinkCube(ent::Phase & aPhase)
{
    auto cube = mBlinkingCube.get(aPhase);
    if((int)mSimulationTime % 2 == 0 && cube)
    {
        cube->erase();
    }
    else if((int)mSimulationTime % 2 == 1 && !cube)
    {
        mBlinkingCube = mWorld.addEntity();
        mBlinkingCube.get(aPhase)->add(component::Geometry{
            .mPosition = { 0.f, 1.f, -3.f},
            .mYRotation = math::Degree<float>{45.f},
        });
    }

}

std::unique_ptr<visu::GraphicState> Cubes::makeGraphicState() const
{
    auto state = std::make_unique<visu::GraphicState>();

    ent::Phase nomutation;
    mQueryRenderable.get(nomutation).each([&state](component::Geometry & aGeometry)
    {
        state->mEntities.push_back(visu::Entity{
            .mPosition_world = aGeometry.mPosition,
            .mYAngle = aGeometry.mYRotation,
            .mColor = math::hdr::gBlue<float>,
        });
    });

    state->mCamera = mSystemOrbitalCamera->getCamera();
    return state;
}


snac::Camera::Parameters Cubes::getCameraParameters() const
{
    return mCameraParameters;
}


Cubes::Renderer_t Cubes::makeRenderer() const
{
    return Renderer_t{
        math::getRatio<float>(mAppInterface->getWindowSize()),
        getCameraParameters(),
    };
}


} // namespace cubes
} // namespace ad
