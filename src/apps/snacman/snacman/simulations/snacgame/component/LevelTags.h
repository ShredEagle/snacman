#pragma once

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

struct Portal
{
    int portalIndex;
};
}
}
}
