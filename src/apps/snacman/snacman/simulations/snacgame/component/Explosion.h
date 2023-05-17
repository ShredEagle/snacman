#pragma once

#include "math/Interpolation/ParameterAnimation.h"
#include <snacman/Timing.h>

namespace ad {
namespace snacgame {
namespace component {

struct Explosion
{
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<float, math::AnimationResult::Clamp> mParameter;
};

} // namespace component
} // namespace snacgame
} // namespace ad
