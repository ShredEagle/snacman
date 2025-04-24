#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mPosition_u));
        aWitness.witness(NVP(mScale));
        aWitness.witness(NVP(mRotationCCW));
    }
};

REFLEXION_REGISTER(PoseScreenSpace)

} // namespace component
} // namespace cubes
} // namespace ad
