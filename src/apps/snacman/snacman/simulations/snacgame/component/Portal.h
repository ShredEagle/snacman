#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <math/Box.h>


namespace ad {
namespace snacgame {
namespace component {
//
// This assume that portal are always on a boundary of the level
// in the x coordinate
constexpr const math::Box<float> gPortalHitbox{{-0.5f, 0.f, -0.5f},
                                               {1.f, 1.f, 1.f}};

struct Portal
{
    int portalIndex = -1;
    math::Position<3, float> mMirrorSpawnPosition;
    math::Box<float> mEnterHitbox;
    math::Box<float> mExitHitbox;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(portalIndex));
        aWitness.witness(NVP(mMirrorSpawnPosition));
        aWitness.witness(NVP(mEnterHitbox));
        aWitness.witness(NVP(mExitHitbox));
    }
};

REFLEXION_REGISTER(Portal)

}
}
}
