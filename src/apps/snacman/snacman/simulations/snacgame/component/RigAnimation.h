#pragma once

#include <snacman/Timing.h>

#include <math/Interpolation/Parameter.h>

#include <snac-renderer/Rigging.h>


namespace ad {
namespace snacgame {
namespace component {

struct RigAnimation
{
    // TODO this pointer is bad design, we need a better handle mechanism for resources
    const snac::NodeAnimation * mAnimation;
    snac::Clock::time_point mStartTime;
    math::Parameter<double, math::FullRange, math::periodic::PingPong> mParameter;
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