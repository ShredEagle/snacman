#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

namespace ad {
namespace snacgame {
namespace component {

struct Pill
{
    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(Pill)

struct LevelTile
{
    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(LevelTile)

// Entity that should be deleted during
// a round transition
struct RoundTransient
{
    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(RoundTransient)

// Entity that should be deleted on
// game scene teardown
struct GameTransient
{
    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(GameTransient)

struct Crown
{
    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(Crown)

} // namespace component
} // namespace snacgame
} // namespace ad
