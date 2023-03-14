#pragma once


#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>


namespace ad {
namespace snacgame {
namespace component {

enum class GeometryLayer
{
    Level = 0,
    Player = 2,
    Pill = 6,
};


struct Geometry
{
    // Pose
    math::Position<2, float> mPosition;
    math::Size<3, float> mScaling = math::Size<3, float>{1.f, 1.f, 1.f};
    GeometryLayer mLayer = GeometryLayer::Level;
    math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
    math::hdr::Rgba_f mColor;
};


} // namespace component
} // namespace cubes
} // namespace ad
