#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

#include <nlohmann/json_fwd.hpp>

namespace ad {
namespace snacgame {
namespace component {

struct Geometry
{
    // Pose
    math::Position<3, float> mPosition = math::Position<3, float>::Zero();
    // Since we only authorize uniform scaling transfer in the scene graph
    // (meaning a scaling that is also applied to the child)
    // mScaling is a single float that we be used
    // like this math::trans3d::scale(mScaling, mScaling, mScaling);
    float mScaling = 1.f;
    // mInstanceScaling is not used in scene graph resolution
    // And thus can be non uniform
    math::Size<3, float> mInstanceScaling = math::Size<3, float>{1.f, 1.f, 1.f};
    math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
    math::hdr::Rgba_f mColor = math::hdr::gWhite<float>;

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

REFLEXION_REGISTER(Geometry)

} // namespace component
} // namespace cubes
} // namespace ad
