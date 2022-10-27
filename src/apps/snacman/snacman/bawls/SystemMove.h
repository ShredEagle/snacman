#pragma once

#include "ComponentGeometry.h"
#include "ComponentVelocity.h"

#include <entity/Query.h>

#include <math/Rectangle.h>


namespace ad {
namespace bawls {
namespace system {

class Move
{
public:
    Move(ent::EntityManager & aWorld, const math::Rectangle<GLfloat> & mBoundingBox) :
        mWorldBounds{mBoundingBox},
        mMovables{aWorld}
    {}

    void update(float aDelta)
    {
        mMovables.each([aDelta, this](component::Geometry & aGeometry, component::Velocity & aVelocity)
            {
                // Position integration
                aGeometry.position += aVelocity * aDelta;

                // Window border bounce
                math::Vec<2, GLfloat> radiusVec{aGeometry.radius, aGeometry.radius};

                // One each dimension:
                // the signed value of the overflow, or zero if there is none.
                math::Vec<2, GLfloat> overflow =
                    (max(mWorldBounds.topRight(), aGeometry.position + radiusVec)
                        - mWorldBounds.topRight())
                    + (min(mWorldBounds.bottomLeft(), aGeometry.position - radiusVec)
                        - mWorldBounds.bottomLeft())
                    ;
                if(overflow != math::Vec<2, GLfloat>::Zero())
                {
                    aGeometry.position -= overflow;
                    if (overflow.x()) aVelocity.x() *= -1.f;
                    if (overflow.y()) aVelocity.y() *= -1.f;
                }
            });
    }

private:
    math::Rectangle<GLfloat> mWorldBounds;
    ent::Query<component::Geometry, component::Velocity> mMovables;
};

} // namespace system
} // namespace bawls
} // namespace ad
