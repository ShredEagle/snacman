#pragma once

#include "math/Vector.h"

namespace ad {
namespace snacgame {
namespace component {

struct Spawner
{
    math::Position<3, float> mSpawnPosition;
    bool mSpawnedPlayer = false;
};


} // namespace component
} // namespace snacgame
} // namespace ad
