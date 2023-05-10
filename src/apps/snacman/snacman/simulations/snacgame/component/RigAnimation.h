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

    double getParameter(snac::Clock::time_point aCurrentTime)
    {
        return mParameter.at(snac::asSeconds(aCurrentTime - mStartTime));
    }
};


} // namespace component
} // namespace snacgame
} // namespace ad