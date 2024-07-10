#pragma once

#include "math/Interpolation/ParameterAnimation.h"
#include <snacman/Timing.h>
#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {

struct Explosion
{
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<float, math::AnimationResult::Clamp> mParameter;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mStartTime);
        archive & SERIAL_PARAM(mParameter);
    }
};

SNAC_SERIAL_REGISTER(Explosion)

} // namespace component
} // namespace snacgame
} // namespace ad
