#pragma once

#include "LightPoint.h" // For LightColors

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Vector.h>
#include <math/Color.h>


namespace ad::snacgame::component {


struct LightDirection
{
    math::UnitVec<3, float> mDirection{
        math::UnitVec<3, float>{{0.f, -1.f, 0.f}}};
    LightColors mColors;
    // Could projectShadow could alternatively be provided as a Tag component,
    // in particular if the API of the rendrer requires sorting shadow light together.
    bool mProjectShadow{false};

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mDirection));
        aWitness.witness(NVP(mColors));
        aWitness.witness(NVP(mProjectShadow));
    }
};

REFLEXION_REGISTER(LightDirection)


} //ad::snacgame::component
