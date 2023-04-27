#pragma once

#include "math/Box.h"
namespace ad {
namespace snacgame {
namespace component {
struct Pill
{};

enum class PowerUpType {
    Dog,
};

struct PowerUp
{
    PowerUpType mType;
    bool mPickedUp = false;
};

struct LevelEntity
{};

struct LevelToCreate
{};

struct LevelCreated
{};
//
// This assume that portal are always on a boundary of the level
// in the x coordinate
constexpr const math::Box<float> gPortalHitbox{{-0.5f, 0.f, -0.5f},
                                               {1.f, 1.f, 1.f}};


struct Portal
{
    int portalIndex;
    math::Position<3, float> mMirrorSpawnPosition;
    math::Box<float> mEnterHitbox;
    math::Box<float> mExitHitbox;
};
}
}
}
