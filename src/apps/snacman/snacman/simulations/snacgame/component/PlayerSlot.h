#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <math/Color.h>

#include <snacman/Input.h>

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct Unspawned
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct PlayerSlot
{
    unsigned int mSlotIndex;
    ent::Handle<ent::Entity> mPlayer;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mSlotIndex));
        aWitness.witness(NVP(mPlayer));
    }
};

REFLEXION_REGISTER(PlayerSlot)

} // namespace component
} // namespace snacgame
} // namespace ad
