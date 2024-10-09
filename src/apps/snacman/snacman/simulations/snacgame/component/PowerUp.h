#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <entity/Entity.h>

#include <math/Quaternion.h>
#include <math/Vector.h>

#include <array>
#include <variant>

namespace ad {
namespace snacgame {
namespace component {

enum class PowerUpType : unsigned int
{
    Dog,
    Teleport,
    Missile,
    None,
    _End = None,
};

constexpr float gMissileSpeed = 5.f;

struct PowerUp
{
    PowerUpType mType = PowerUpType::None;
    float mSwapPeriod = 1.f;
    float mSwapTimer = mSwapPeriod;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mSwapPeriod));
        aWitness.witness(NVP(mSwapTimer));
    }
};

REFLEXION_REGISTER(PowerUp)

struct InGameDog
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

constexpr float gMissileDelayFlashing = 4.f;
constexpr float gMissileDelayExplosion = 5.f;

struct InGameMissile
{
    ent::Handle<ent::Entity> mModel;
    ent::Handle<ent::Entity> mDamageArea;
    float mDelayExplosion = gMissileDelayExplosion;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mModel));
        aWitness.witness(NVP(mDamageArea));
        aWitness.witness(NVP(mDelayExplosion));
    }
};

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
    std::variant<InGameMissile, InGameDog> mInfo;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mOwner));
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mInfo));
    }
};

REFLEXION_REGISTER(InGamePowerup)

} // namespace component
} // namespace snacgame
} // namespace ad
