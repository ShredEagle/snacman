#pragma once

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
    Bomb,
    None,
    _End = None,
};

constexpr float gMissileSpeed = 5.f;

struct PowerUp
{
    PowerUpType mType = PowerUpType::None;
    float mSwapPeriod = 1.f;
    float mSwapTimer = mSwapPeriod;
};

struct InGameDog
{
};

constexpr float gMissileDelayFlashing = 4.f;
constexpr float gMissileDelayExplosion = 5.f;

struct InGameMissile
{
    ent::Handle<ent::Entity> mModel;
    ent::Handle<ent::Entity> mDamageArea;
    float mDelayExplosion = gMissileDelayExplosion;
};

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
    std::variant<InGameMissile, InGameDog> mInfo;
};

} // namespace component
} // namespace snacgame
} // namespace ad
