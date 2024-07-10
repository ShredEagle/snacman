#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include <math/Angle.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace component {


struct PoseScreenSpace
{
    // Pose
    math::Position<2, float> mPosition_u;
    math::Size<2, float> mScale{1.f, 1.f};
    math::Radian<float> mRotationCCW{0.f};
    // It is difficult to work with a single transformation matrix, because we want to express position in screen unit square
    // (and atm, the transformation in the shader should result in pixel coordinates)
    // Plus for non-origin aligned rotations, it is hard to express the origin in screen unit square...
    //math::AffineMatrix<3, float> mLocalToScreenUnit{math::AffineMatrix<3, float>::Identity}; // screen unit square, from [-1, -1] to [1, 1]

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mPosition_u);
        archive & SERIAL_PARAM(mScale);
        archive & SERIAL_PARAM(mRotationCCW);
    }
};

SNAC_SERIAL_REGISTER(PoseScreenSpace)


} // namespace component
} // namespace cubes
} // namespace ad
