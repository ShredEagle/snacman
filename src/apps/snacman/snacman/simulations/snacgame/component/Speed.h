#pragma once

#include <math/Quaternion.h>

namespace ad {
namespace snacgame {


// TODO should be moved to math
// and 3DMPGGD suggest even to use 'exponential map' instead (less singularities)
// Axis angle / exponential map is the recommanded model for orientation rate of change (angular velocity)
struct AxisAngle
{
    math::UnitVec<3, float> mAxis;
    math::Radian<float> mAngle;
};


namespace component {

struct Speed
{
    // Note: Quaternion would be a great approach if we guarantee locked time steps.
    // Yet an angular displacement quaternion cannot cheaply be "stepped" with time
    //math::Quaternion<float> mRotation;

   AxisAngle mRotation; 
};


} // namespace component
} // namespace cubes
} // namespace ad

