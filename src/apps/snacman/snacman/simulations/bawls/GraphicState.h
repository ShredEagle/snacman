#pragma once


#include "ComponentGeometry.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <graphics/2d/Shaping.h>

#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>


namespace ad {
namespace bawls {


struct GraphicState
{
    std::vector<graphics::r2d::Shaping::Circle> balls;
};


class GraphicStateProducer
{
public:
    GraphicStateProducer(ent::EntityManager & aWorld) :
        mRenderables{aWorld}
    {}

    std::unique_ptr<GraphicState> makeGraphicState() //const TODO should allow constness
    {
        auto state = std::make_unique<GraphicState>();

        mRenderables.each([&state](const component::Geometry & aGeometry)
            {
                state->balls.push_back({math::Position<3, GLfloat>{aGeometry.position, 0.f}, aGeometry.radius});
            });

        return state;
    }

private:
    ent::Query<component::Geometry> mRenderables;
};


inline GraphicState interpolate(const GraphicState & aLeft, const GraphicState & aRight, float aInterpolant)
{
    GraphicState state;
    for (std::size_t ballId = 0; ballId != aLeft.balls.size(); ++ballId)
    {
        state.balls.emplace_back(
            math::lerp(aLeft.balls[ballId].mPosition_world, aRight.balls[ballId].mPosition_world, aInterpolant),
            // The size remains constant
            aLeft.balls[ballId].mSize_world.width());
    }
    return state;
}


} // namespace bawls
} // namespace ad
