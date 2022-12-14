#pragma once


#include <math/Color.h>
#include <math/Angle.h>
#include <math/Vector.h>


namespace ad {
namespace snacgame {
namespace component {


struct Geometry
{
    // Pose
    math::Position<3, float> mPosition;
    math::Radian<float> mYRotation;
    math::hdr::Rgba_f mColor;
    
    //Metadata
    bool mShouldBeDrawn = true;
};


} // namespace component
} // namespace cubes
} // namespace ad
