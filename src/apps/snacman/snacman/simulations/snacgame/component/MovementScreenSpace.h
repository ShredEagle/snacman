#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Angle.h>

namespace ad {
namespace snacgame {
namespace component {


struct MovementScreenSpace
{
    math::Radian<float> mAngularSpeed; // radian/second

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mAngularSpeed));
    }
};

REFLEXION_REGISTER(MovementScreenSpace)

} // namespace component
} // namespace cubes
} // namespace ad
