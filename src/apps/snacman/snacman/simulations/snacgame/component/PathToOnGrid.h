#pragma once

#include <entity/Entity.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace component {

struct PathToOnGrid
{
    ent::Handle<ent::Entity> mEntityTarget;
    math::Position<2, int> mCurrentTarget;
};

} // namespace component
} // namespace snacgame
} // namespace ad
