#pragma once


namespace ad {
namespace snacgame {
namespace component {

constexpr float gBaseTimeToRespawn = 2.f;
constexpr float gBaseInvulFrameDuration = 2.f;

struct PlayerLifeCycle
{
    int mPoints = 0;
    bool mIsAlive = false;
    float mTimeToRespawn = 0;
    float mInvulFrameCounter = 0;
};


} // namespace component
} // namespace snacgame
} // namespace ad
