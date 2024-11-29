#pragma once

#include "snacman/Timing.h"
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
    ent::Handle<ent::Entity> mPill;
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

struct BurgerLossHitbox
{
    ent::Handle<ent::Entity> mTile = {};

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

REFLEXION_REGISTER(BurgerLossHitbox)

struct BurgerParticle
{
    math::Position<3, float> mTargetPos;
    float mTargetHeight = 0.f;
    float mTargetNorm = 0.f;
    float mBaseSpeed = 0.f;
    snac::Clock::time_point mStartTime;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mTargetPos));
    }
};

REFLEXION_REGISTER(BurgerParticle)

} // namespace component
} // namespace snacgame
} // namespace ad
