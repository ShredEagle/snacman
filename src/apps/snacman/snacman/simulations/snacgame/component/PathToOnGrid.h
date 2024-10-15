#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <entity/Entity.h>
#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace component {

struct PathToOnGrid
{
    ent::Handle<ent::Entity> mEntityTarget;
    math::Position<2, float> mCurrentTarget;
    bool mTargetFound = false;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mEntityTarget));
        aWitness.witness(NVP(mCurrentTarget));
        aWitness.witness(NVP(mTargetFound));
    }
};

REFLEXION_REGISTER(PathToOnGrid)

} // namespace component
} // namespace snacgame
} // namespace ad
