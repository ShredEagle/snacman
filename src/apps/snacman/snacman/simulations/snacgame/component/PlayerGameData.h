#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct PlayerGameData
{
    // Kept alive between rounds
    int mRoundsWon = 0;

    // The HUD showing player informations (score, power-up)
    ent::Handle<ent::Entity> mHud;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mRoundsWon));
        aWitness.witness(NVP(mHud));
    }
};

REFLEXION_REGISTER(PlayerGameData)

} // namespace component
} // namespace snacgame
} // namespace ad
