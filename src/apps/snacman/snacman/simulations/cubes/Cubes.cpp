#include "Cubes.h"

#include "SystemMove.h"


namespace ad {
namespace cubes {


Cubes::Cubes() :
    mSystemMove{mWorld.addEntity()},
    mQueryRenderable{mWorld}
{
    ent::Phase init;
    mSystemMove.get(init)->add(system::Move{mWorld});

    // Add cubes
    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = { 3.f, 0.f, -5.f},
        .mYRotation = math::Degree{0.f},
    });

    mWorld.addEntity().get(init)->add(component::Geometry{
        .mPosition = {-3.f, 0.f, -5.f},
        .mYRotation = math::Degree{0.f},
    });
}


void Cubes::update(float aDelta)
{
    ent::Phase update;
    mSystemMove.get(update)->get<system::Move>().update(aDelta);
}


std::unique_ptr<GraphicState> Cubes::makeGraphicState() const
{
    auto state = std::make_unique<GraphicState>();

    ent::Phase nomutation;
    mQueryRenderable.get(nomutation).each([&state](component::Geometry & aGeometry)
    {
        state->mEntities.push_back(Entity{
            .mPosition_world = aGeometry.mPosition,
            .mYAngle = aGeometry.mYRotation,
            .mColor = math::hdr::gBlue<float>,
        });
    });

    state->mCamera.mPosition_world = {0.f, 0.f, 5.f};
    return state;
}


} // namespace cubes
} // namespace ad
