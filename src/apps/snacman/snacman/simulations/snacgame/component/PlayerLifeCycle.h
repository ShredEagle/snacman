#pragma once


#include <entity/Entity.h>


namespace ad {
namespace snacgame {
namespace component {

constexpr float gBaseTimeToRespawn = 2.f;
constexpr float gBaseInvulFrameDuration = 2.f;

constexpr float gBaseHitStunDuration = 2.f;

struct PlayerLifeCycle
{
    void reset()
    {
        // Do not lose the hud reference though
        auto hudCopy = std::move(mHud);
        *this = PlayerLifeCycle{};
        mHud = std::move(hudCopy);
    }

    int mScore = 0;
    bool mIsAlive = false;
    float mTimeToRespawn = 0;
    float mInvulFrameCounter = 0;

    // Hit stun
    float mHitStun = 0;

    // The HUD showing player informations (score, power-up)
    std::optional<ent::Handle<ent::Entity>> mHud;
};


} // namespace component
} // namespace snacgame
} // namespace ad
