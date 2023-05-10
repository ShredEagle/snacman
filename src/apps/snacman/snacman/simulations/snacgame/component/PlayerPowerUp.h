#pragma once

#include "PowerUp.h"

#include <entity/Entity.h>
#include <variant>

namespace ad {
namespace snacgame {
namespace component {

constexpr float gTeleportChangeTargetDelay = 1.f;

struct DogPowerUpInfo
{
};

struct TeleportPowerUpInfo
{
    std::optional<ent::Handle<ent::Entity>> mCurrentTarget;
    float mDelayChangeTarget = 0.f;
    std::optional<ent::Handle<ent::Entity>> mTargetArrow;
};

struct PlayerPowerUp
{
    ent::Handle<ent::Entity> mPowerUp;
    PowerUpType mType;
    std::variant<DogPowerUpInfo, TeleportPowerUpInfo> mInfo;
};

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
};

} // namespace component
} // namespace snacgame
} // namespace ad
