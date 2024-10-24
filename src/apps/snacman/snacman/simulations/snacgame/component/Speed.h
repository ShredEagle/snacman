#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Quaternion.h>

namespace ad {
namespace snacgame {

// TODO should be moved to math
// and 3DMPGGD suggest even to use 'exponential map' instead (less
// singularities) Axis angle / exponential map is the recommanded model for
// orientation rate of change (angular velocity)
struct AxisAngle
{
    math::UnitVec<3, float> mAxis;
    math::Radian<float> mAngle;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mAxis));
        aWitness.witness(NVP(mAngle));
    }
};

namespace component {

struct Speed
{
    // Note: Quaternion would be a great approach if we guarantee locked time
    // steps. Yet an angular displacement quaternion cannot cheaply be "stepped"
    // with time
    // math::Quaternion<float> mRotation;
    math::Vec<3, float> mSpeed = {0.f, 0.f, 0.f};
    AxisAngle mRotation = {
        .mAxis = math::UnitVec<3, float>::MakeFromUnitLength({1.f, 0.f, 0.f}),
        .mAngle = math::Radian<float>{0.f}};

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mSpeed));
        aWitness.witness(NVP(mRotation));
    }
};

REFLEXION_REGISTER(Speed)

} // namespace component
} // namespace snacgame
} // namespace ad
