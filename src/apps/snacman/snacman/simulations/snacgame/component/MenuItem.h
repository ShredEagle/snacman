#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include "../scene/Scene.h"

#include <unordered_map>

namespace ad {
namespace snacgame {
namespace component {

struct MenuItem
{
    std::string mName;
    bool mSelected;

    std::unordered_map<int, std::string> mNeighbors;
    scene::Transition mTransition;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mName));
        aWitness.witness(NVP(mSelected));
        aWitness.witness(NVP(mNeighbors));
        aWitness.witness(NVP(mTransition));
    }
};

REFLEXION_REGISTER(MenuItem)

} // namespace component
} // namespace snacgame
} // namespace ad
