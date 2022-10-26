#pragma once


#include "ComponentGeometry.h"

#include "../GraphicState.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <graphics/2d/Shaping.h>

#include <math/Vector.h>


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

} // namespace bawls
} // namespace ad
