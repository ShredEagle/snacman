#pragma once


#include <math/Quaternion.h>


namespace ad {
namespace snacgame {
namespace component {

// TODO should be moved to math
// and 3DMPGGD suggest even to use 'exponential map' instead (less singularities)
// Axis angle / exponential map is the recommanded model for orientation rate of change (angular displacement speed)
struct AxisAngle
{
    math::UnitVec<3, float> mAxis;
    math::Radian<float> mAngle;
};

struct Speed
{
    // Note: would be a great approach if we guarantee locked time steps
    // but an angular displacement quaternion cannot cheaply be "stepped" with time
    math::Quaternion<float> mRotation;
};


} // namespace component
} // namespace cubes
} // namespace ad

