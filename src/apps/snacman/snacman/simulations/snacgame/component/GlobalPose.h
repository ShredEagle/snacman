#pragma once

#include <math/Vector.h>
#include <math/Quaternion.h>
#include <math/Color.h>
#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mPosition);
        archive & SERIAL_PARAM(mScaling);
        archive & SERIAL_PARAM(mInstanceScaling);
        archive & SERIAL_PARAM(mOrientation);
        archive & SERIAL_PARAM(mColor);
    }
};

SNAC_SERIAL_REGISTER(GlobalPose)

}
}
}
