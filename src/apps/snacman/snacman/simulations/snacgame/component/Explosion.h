#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include "math/Interpolation/ParameterAnimation.h"
#include <snacman/Timing.h>

namespace ad {
namespace snacgame {
namespace component {

struct Explosion
{
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<float, math::AnimationResult::Clamp> mParameter = math::ParameterAnimation<float, math::AnimationResult::Clamp>{0.f};

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mStartTime));
        aWitness.witness(NVP(mParameter));
    }
};

REFLEXION_REGISTER(Explosion)

} // namespace component
} // namespace snacgame
} // namespace ad
