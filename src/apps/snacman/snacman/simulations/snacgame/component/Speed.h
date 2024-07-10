#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mAxis);
        archive & SERIAL_PARAM(mAngle);
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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mSpeed);
        archive & SERIAL_PARAM(mRotation);
    }
};

SNAC_SERIAL_REGISTER(Speed)

} // namespace component
} // namespace snacgame
} // namespace ad
