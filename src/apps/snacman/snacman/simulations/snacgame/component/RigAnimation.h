#pragma once

#include <snacman/Timing.h>

#include <math/Interpolation/ParameterAnimation.h>

#include <snac-renderer/Rigging.h>


namespace ad {
namespace snacgame {
namespace component {

struct RigAnimation
{
    // TODO this pointer is bad design, we need a better handle mechanism for resources
    std::string mAnimName = "";
    const snac::NodeAnimation * mAnimation;
    const std::unordered_map<std::string, snac::NodeAnimation> * mAnimationMap;
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<double, math::FullRange, math::periodic::Repeat> mParameter;
    // TODO this is a somehow a duplication, because we need the resulting parameter value to make the graphic state.
    double mParameterValue;

    double getParameter(snac::Clock::time_point aCurrentTime) const
    {
        return mParameter.at(snac::asSeconds(aCurrentTime - mStartTime));
    }
};


} // namespace component
} // namespace snacgame
} // namespace ad
