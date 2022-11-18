#include "Cubes.h"

#include "SystemMove.h"

#include <math/VectorUtilities.h>


namespace ad {
namespace cubes {


Cubes::Cubes(graphics::AppInterface & aAppInterface) :
    mAppInterface{&aAppInterface},
    mSystemMove{mWorld.addEntity()},
    mSystemOrbitalCamera{mWorld, mWorld},
    mQueryRenderable{mWorld, mWorld}
{
    ent::Phase init;
    mSystemMove.get(init)->add(system::Move{mWorld});

    // Add cubes
    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = { 1.5f, 0.f, 0.f},
        .mYRotation = math::Degree{0.f},
    });

    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = {-1.5f, 0.f, 0.f},
        .mYRotation = math::Degree{0.f},
    });
}


void Cubes::update(float aDelta, const snac::Input & aInput)
{
    ent::Phase update;
    mSystemMove.get(update)->get<system::Move>().update(aDelta);
    mSystemOrbitalCamera->update(aInput,
                                 mCameraParams.vFov,
                                 mAppInterface->getWindowSize().height());
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


Cubes::Renderer_t Cubes::makeRenderer() const
{
    return Renderer_t{
        math::getRatio<float>(mAppInterface->getWindowSize()),
        mCameraParams
    };
}


} // namespace cubes
} // namespace ad
