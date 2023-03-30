#pragma once

#include <entity/Entity.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace component {

struct PathToOnGrid
{
    ent::Handle<ent::Entity> mEntityTarget;
    math::Position<2, float> mCurrentTarget;
    bool mTargetFound = false;
};

} // namespace component
} // namespace snacgame
} // namespace ad
