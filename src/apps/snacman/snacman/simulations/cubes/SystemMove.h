#pragma once

#include "ComponentGeometry.h"

#include <entity/Query.h>


namespace ad {
namespace cubes {
namespace system {

class Move
{
public:
    Move(ent::EntityManager & aWorld) :
        mMovables{aWorld}
    {}

    void update(float aDelta);


private:
    static constexpr math::Radian<float> gAngularSpeed{math::pi<float> / 4};

    ent::Query<component::Geometry> mMovables;
};

} // namespace system
} // namespace cubes
} // namespace ad
