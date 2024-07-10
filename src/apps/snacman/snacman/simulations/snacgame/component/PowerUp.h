#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mType);
        archive & SERIAL_PARAM(mSwapPeriod);
        archive & SERIAL_PARAM(mSwapTimer);
    }
};

SNAC_SERIAL_REGISTER(PowerUp)

struct InGameDog
{

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(InGameDog)

constexpr float gMissileDelayFlashing = 4.f;
constexpr float gMissileDelayExplosion = 5.f;

struct InGameMissile
{
    ent::Handle<ent::Entity> mModel;
    ent::Handle<ent::Entity> mDamageArea;
    float mDelayExplosion = gMissileDelayExplosion;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mModel);
        archive & SERIAL_PARAM(mDamageArea);
        archive & SERIAL_PARAM(mDelayExplosion);
    }
};

SNAC_SERIAL_REGISTER(InGameMissile)

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
    std::variant<InGameMissile, InGameDog> mInfo;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mOwner);
        archive & SERIAL_PARAM(mType);
        archive & SERIAL_PARAM(mInfo);
    }
};

SNAC_SERIAL_REGISTER(InGamePowerup)

} // namespace component
} // namespace snacgame
} // namespace ad
