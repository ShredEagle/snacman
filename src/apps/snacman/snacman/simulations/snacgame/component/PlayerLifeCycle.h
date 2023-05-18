#pragma once


#include <entity/Entity.h>


namespace ad {
namespace snacgame {
namespace component {

constexpr float gBaseTimeToRespawn = 2.f;
constexpr float gBaseInvulFrameDuration = 0.5f;

constexpr float gBaseHitStunDuration = 2.f;

struct PlayerLifeCycle
{
    void resetBetweenRound()
    {
        // Do not lose the hud reference nor the count of rounds won
        mScore = 0;
        mIsAlive = false;
        mTimeToRespawn = 0.f;
        mInvulFrameCounter = gBaseInvulFrameDuration;
        mHitStun = 0.f;
    }

    int mScore = 0;
    bool mIsAlive = false;
    float mTimeToRespawn = 0;
    float mInvulFrameCounter = 0;
    // Hit stun
    float mHitStun = 0;

    // Kept alive between rounds
    int mRoundsWon = 0;

    // The HUD showing player informations (score, power-up)
    std::optional<ent::Handle<ent::Entity>> mHud;
};


} // namespace component
} // namespace snacgame
} // namespace ad
