#pragma once

#include "snacman/serialization/Witness.h"

#include <math/Interpolation/ParameterAnimation.h>
#include <reflexion/NameValuePair.h>
#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <snac-renderer-V2/Handle.h>
#include <snac-renderer-V2/Rigging.h>
#include <snacman/Timing.h>

namespace ad {
namespace snacgame {
namespace component {

// TODO(franz): The RigAnimation component is not good
// The RigAnimation is completely dependent on the model
// it is retrieved from the model and loaded with the model
// This makes that you cannot add a RigAnimation component without
// Adding a VisualModel first which doesn't seem right
struct RigAnimation
{
    renderer::Handle<const renderer::RigAnimation> mAnimation;
    // Used to get to the map of RigAnimations
    renderer::Handle<const renderer::AnimatedRig> mAnimatedRig;
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<double, math::FullRange, math::periodic::Repeat>
        mParameter =
            math::ParameterAnimation<double, math::FullRange,math::periodic::Repeat>{0.f};

    // TODO this is a somehow a duplication, because we need the resulting
    // parameter value to make the graphic state.
    float mParameterValue; // A float should be enough, we expect animations to
                           // run for a few minutes at maximum.

    double getParameter(snac::Clock::time_point aCurrentTime) const
    {
        return mParameter.at(snac::asSeconds(aCurrentTime - mStartTime));
    }

    template <class T_witness>
    void describeTo(T_witness && aWitness)
    {
        // NOTE: It doesn't make sense to serialize
        // the animation component because its data is 
        // completely dependent on the model component
        // of the same handle
    }
};

REFLEXION_REGISTER(RigAnimation)

} // namespace component
} // namespace snacgame
} // namespace ad
