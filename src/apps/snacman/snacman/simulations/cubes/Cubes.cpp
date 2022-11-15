#include "Cubes.h"

namespace ad {
namespace cubes {


void Cubes::update(float aDelta)
{

}


std::unique_ptr<GraphicState> Cubes::makeGraphicState() const
{
    auto state = std::make_unique<GraphicState>();
    state->mEntities.push_back(Entity{
        .mPosition_world = math::Position<3, float>{0.f, 0.f, 0.f},
        .mYAngle = math::Degree<float>{60.f},
        .mColor = math::hdr::gBlue<float>,
    });
    state->mCamera.mPosition_world = {0.f, 0.f, 5.f};
    return state;
}


} // namespace cubes
} // namespace ad
