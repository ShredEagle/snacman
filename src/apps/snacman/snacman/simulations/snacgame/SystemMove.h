#pragma once

#include "component/Geometry.h"

#include <entity/Query.h>


namespace ad {
namespace snacgame {
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
