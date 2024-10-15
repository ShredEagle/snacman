#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include "math/Vector.h"

namespace ad {
namespace snacgame {
namespace component {

struct Spawner
{
    math::Position<3, float> mSpawnPosition;
    bool mSpawnedPlayer = false;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mSpawnPosition));
        aWitness.witness(NVP(mSpawnedPlayer));
    }
};

REFLEXION_REGISTER(Spawner)

} // namespace component
} // namespace snacgame
} // namespace ad
