#pragma once


#include <math/Color.h>
#include <math/Angle.h>
#include <math/Vector.h>


namespace ad {
namespace snacgame {
namespace component {

enum class GeometryLayer
{
    Level,
    Player,
};


struct Geometry
{
    // Pose
    math::Position<2, float> mSubGridPosition;
    math::Position<2, int> mGridPosition;
    GeometryLayer mLayer = GeometryLayer::Level;
    math::Radian<float> mYRotation;
    math::hdr::Rgba_f mColor;
    
    //Metadata
    bool mShouldBeDrawn = true;
};


} // namespace component
} // namespace cubes
} // namespace ad
