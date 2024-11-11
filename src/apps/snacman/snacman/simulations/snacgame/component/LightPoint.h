#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Vector.h>
#include <math/Color.h>


namespace ad::snacgame::component {


// TODO move as renderer's light implementation
struct LightColors
{
    alignas(16) math::hdr::Rgb<GLfloat> mDiffuse = math::hdr::gWhite<GLfloat>;
    alignas(16) math::hdr::Rgb<GLfloat> mSpecular = math::hdr::gWhite<GLfloat>;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mDiffuse));
        aWitness.witness(NVP(mSpecular));
    }
};


struct LightPoint
{
    // Note: no position, it should be handled via a GlobalPose component.
    renderer::Radius mRadius;
    LightColors mColors;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mRadius));
        aWitness.witness(NVP(mColors));
    }

    template<class T_witness>
    static LightPoint construct(T_witness && aWitness)
    {
        renderer::Radius r{0.f,0.f};
        LightColors color;
        aWitness.witness(NVP_NAMED("mRadius", r));
        aWitness.witness(NVP_NAMED("mColor", color));
        return LightPoint{r, color};

    }
};

REFLEXION_REGISTER(LightPoint)


} //ad::snacgame::component
