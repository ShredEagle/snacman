#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <math/Vector.h>
#include <math/Quaternion.h>
#include <math/Color.h>

namespace ad {
namespace snacgame {
namespace component {

struct GlobalPose
{
    math::Position<3, float> mPosition;
    // TODO rename mUniformScaling
    // TODO Use a Pose struct directly here!
    float mScaling = 1.f;
    math::Size<3, float> mInstanceScaling = math::Size<3, float>{1.f, 1.f, 1.f};
    math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
    math::hdr::Rgba_f mColor; // TODO should not be here

    void drawUi() const;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mPosition));
        aWitness.witness(NVP(mScaling));
        aWitness.witness(NVP(mInstanceScaling));
        aWitness.witness(NVP(mOrientation));
        aWitness.witness(NVP(mColor));
    }
};

REFLEXION_REGISTER(GlobalPose)

}
}
}
