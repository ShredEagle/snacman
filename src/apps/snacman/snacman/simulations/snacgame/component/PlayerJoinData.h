#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {
struct PlayerJoinData
{
    ent::Handle<ent::Entity> mJoinPlayerModel;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mJoinPlayerModel));
    }
};

REFLEXION_REGISTER(PlayerJoinData)

} // namespace component
} // namespace snacgame
} // namespace ad
