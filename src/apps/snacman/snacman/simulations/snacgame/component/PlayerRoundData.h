#pragma once

#include "PowerUp.h"

#include "../GameParameters.h"
#include "../InputConstants.h"

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(DogPowerUpInfo)

struct TeleportPowerUpInfo
{
    ent::Handle<ent::Entity> mCurrentTarget;
    // This is 0.f because we want the teleport to change target
    // as soon as the player inputs a target change
    // and then to wait for the delay
    float mDelayChangeTarget = 0.f;
    ent::Handle<ent::Entity> mTargetArrow;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mCurrentTarget);
        archive & SERIAL_PARAM(mDelayChangeTarget);
        archive & SERIAL_PARAM(mTargetArrow);
    }
};

SNAC_SERIAL_REGISTER(TeleportPowerUpInfo)

struct MissilePowerUpInfo
{

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(MissilePowerUpInfo)

struct PlayerPowerUp
{

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(PlayerPowerUp)

struct ControllingMissile
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(ControllingMissile)

struct PlayerRoundData
{
    int mRoundScore = 0;
    float mInvulFrameCounter = gBaseInvulFrameDuration;

    //Movement data
    int mMoveState = gPlayerMoveFlagNone;

    // Player model this is used to isolate a player
    // scene node from the orientation of the model
    ent::Handle<ent::Entity> mModel;
    ent::Handle<ent::Entity> mCrown;

    // We need access to playerGameData and the slot index
    // which are in the slot
    ent::Handle<ent::Entity> mSlot;

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mRoundScore);
        archive & SERIAL_PARAM(mInvulFrameCounter);
        archive & SERIAL_PARAM(mMoveState);
        archive & SERIAL_PARAM(mModel);
        archive & SERIAL_PARAM(mCrown);
        archive & SERIAL_PARAM(mSlot);
        archive & SERIAL_PARAM(mPowerUp);
        archive & SERIAL_PARAM(mType);
        archive & SERIAL_PARAM(mInfo);
        archive & SERIAL_PARAM(mPortalImage);
        archive & SERIAL_PARAM(mCurrentPortalPos);
        archive & SERIAL_PARAM(mCurrentPortal);
        archive & SERIAL_PARAM(mDestinationPortal);
    }
};

SNAC_SERIAL_REGISTER(PlayerRoundData)

} // namespace component
} // namespace snacgame
} // namespace ad
