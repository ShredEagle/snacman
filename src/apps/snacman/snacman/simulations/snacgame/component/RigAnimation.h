#pragma once

#include <snacman/Timing.h>

#include <math/Interpolation/ParameterAnimation.h>

#include <snac-renderer-V2/Handle.h>
#include <snac-renderer-V2/Rigging.h>


namespace ad {
namespace snacgame {
namespace component {

struct RigAnimation
{
    renderer::Handle<const renderer::RigAnimation> mAnimation;
    // Used to get to the map of RigAnimations
    renderer::Handle<const renderer::AnimatedRig> mAnimatedRig;
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<double, math::FullRange, math::periodic::Repeat> mParameter;

    // TODO this is a somehow a duplication, because we need the resulting parameter value to make the graphic state.
    float mParameterValue; // A float should be enough, we expect animations to run for a few minutes at maximum.

    double getParameter(snac::Clock::time_point aCurrentTime) const
    {
        return mParameter.at(snac::asSeconds(aCurrentTime - mStartTime));
    }
};


} // namespace component
} // namespace snacgame
} // namespace ad
