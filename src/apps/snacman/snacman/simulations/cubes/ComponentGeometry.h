#pragma once


#include <math/Angle.h>
#include <math/Vector.h>


namespace ad {
namespace cubes {
namespace component {


struct Geometry
{
    // Pose
    math::Position<3, float> mPosition;
    math::Radian<float> mYRotation;
};


} // namespace component
} // namespace cubes
} // namespace ad
