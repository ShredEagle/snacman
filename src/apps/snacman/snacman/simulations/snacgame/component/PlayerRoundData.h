#pragma once

#include "PowerUp.h"

#include "../GameParameters.h"
#include "../InputConstants.h"

#include <entity/Entity.h>

#include <variant>

namespace ad {
namespace snacgame {
namespace component {

constexpr float gBaseInvulFrameDuration = 0.5f;
constexpr float gBaseHitStunDuration = 2.f;

constexpr float gTeleportChangeTargetDelay = 1.f;

struct DogPowerUpInfo
{
};

struct TeleportPowerUpInfo
{
    ent::Handle<ent::Entity> mCurrentTarget;
    // This is 0.f because we want the teleport to change target
    // as soon as the player inputs a target change
    // and then to wait for the delay
    float mDelayChangeTarget = 0.f;
    ent::Handle<ent::Entity> mTargetArrow;
};

struct MissilePowerUpInfo
{
};

struct PlayerPowerUp
{
};

struct ControllingMissile
{
};

struct PlayerRoundData
{
    int mRoundScore = 0;
    float mInvulFrameCounter = gBaseInvulFrameDuration;

    //Movement data
    int mMoveState = gPlayerMoveFlagNone;

    // Player model this is used to isolate a player
    // scene node from the orientation of the model
    ent::Handle<ent::Entity> mModel;

    // Power up info
    ent::Handle<ent::Entity> mPowerUp;
    PowerUpType mType = PowerUpType::None;
    std::variant<DogPowerUpInfo, TeleportPowerUpInfo, MissilePowerUpInfo> mInfo;

    // Portal data
    ent::Handle<ent::Entity> mPortalImage;
    math::Position<2, float> mCurrentPortalPos;
    int mCurrentPortal = -1;
    int mDestinationPortal = -1;

    void drawUi() const;
};


} // namespace component
} // namespace snacgame
} // namespace ad
