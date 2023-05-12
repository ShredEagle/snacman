#pragma once


namespace ad {
namespace snacgame {
namespace component {

constexpr float gBaseTimeToRespawn = 2.f;
constexpr float gBaseInvulFrameDuration = 2.f;

constexpr float gBaseHitStunDuration = 2.f;

struct PlayerLifeCycle
{
    int mScore = 0;
    bool mIsAlive = false;
    float mTimeToRespawn = 0;
    float mInvulFrameCounter = 0;

    // Hit stun
    float mHitStun = 0; 
};


} // namespace component
} // namespace snacgame
} // namespace ad
