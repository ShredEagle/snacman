#pragma once

#include "math/Interpolation/ParameterAnimation.h"
#include "snacman/Timing.h"

#include <math/Color.h>
#include <reflexion/NameValuePair.h>
#include <snac-renderer-V1/text/Text.h>
#include <snacman/serialization/Serial.h>
#include <string>

namespace ad {
namespace snacgame {
namespace component {

struct Text
{
    std::string mString;
    std::shared_ptr<snac::Font> mFont;
    math::hdr::Rgba_f mColor;

    template <class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mString));
        aWitness.witness(NVP(mFont));
        aWitness.witness(NVP(mColor));
    }
};

REFLEXION_REGISTER(Text)

struct TextZoom
{
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<float,
                             math::AnimationResult::FullRange,
                             math::None,
                             math::ease::MassSpringDamper>
        mParameter{
            math::ease::MassSpringDamper<float>(
                        1.f,
                        1.2f,
                        1.f
            ),
            1.f
        };

    template <class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mStartTime));
        aWitness.witness(NVP(mParameter));
    }
};

REFLEXION_REGISTER(TextZoom)

} // namespace component
} // namespace snacgame
} // namespace ad
