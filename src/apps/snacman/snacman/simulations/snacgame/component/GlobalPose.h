#pragma once

#include <math/Vector.h>
#include <math/Quaternion.h>
#include <math/Color.h>

namespace ad {
namespace snacgame {
namespace component {

struct GlobalPose
{
    math::Position<3, float> mPosition;
    float mScaling = 1.f;
    math::Size<3, float> mInstanceScaling = math::Size<3, float>{1.f, 1.f, 1.f};
    math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
    math::hdr::Rgba_f mColor; // TODO should not be here

    void drawUi() const;
};
}
}
}
