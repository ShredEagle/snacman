#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct TeleportPowerUpInfo
{
    ent::Handle<ent::Entity> mCurrentTarget;
    // This is 0.f because we want the teleport to change target
    // as soon as the player inputs a target change
    // and then to wait for the delay
    float mDelayChangeTarget = 0.f;
    ent::Handle<ent::Entity> mTargetArrow;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mCurrentTarget));
        aWitness.witness(NVP(mDelayChangeTarget));
        aWitness.witness(NVP(mTargetArrow));
    }
};

struct MissilePowerUpInfo
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct PlayerPowerUp
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct ControllingMissile
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct PlayerRoundData
{
    int mRoundScore = 0;
    float mInvulFrameCounter = 0;

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mRoundScore));
        aWitness.witness(NVP(mInvulFrameCounter));
        aWitness.witness(NVP(mMoveState));
        aWitness.witness(NVP(mModel));
        aWitness.witness(NVP(mCrown));
        aWitness.witness(NVP(mSlot));
        aWitness.witness(NVP(mPowerUp));
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mInfo));
        aWitness.witness(NVP(mPortalImage));
        aWitness.witness(NVP(mCurrentPortalPos));
        aWitness.witness(NVP(mCurrentPortal));
        aWitness.witness(NVP(mDestinationPortal));
    }
};

REFLEXION_REGISTER(PlayerRoundData)

} // namespace component
} // namespace snacgame
} // namespace ad
