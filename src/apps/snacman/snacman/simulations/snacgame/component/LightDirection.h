#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Vector.h>
#include <math/Color.h>


namespace ad::snacgame::component {


struct LightDirection
{
    math::UnitVec<3, float> mDirection{
        math::UnitVec<3, float>{{0.f, -1.f, 0.f}}};
    math::hdr::Rgb_f mColor;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mDirection));
        aWitness.witness(NVP(mColor));
    }
};

REFLEXION_REGISTER(LightDirection)


} //ad::snacgame::component
