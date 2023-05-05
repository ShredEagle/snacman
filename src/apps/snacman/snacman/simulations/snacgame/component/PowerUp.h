#pragma once
#include <array>
namespace ad {
namespace snacgame {
namespace component {

enum class PowerUpType : unsigned int
{
    Dog,
    Teleport,
    _last,
};

constexpr std::array<const char *, static_cast<unsigned int>(PowerUpType::_last)> gPowerupPathByType{
    "models/collar/collar.gltf",
    "models/teleport/teleport.gltf",
};

struct PowerUp
{
    PowerUpType mType;
    bool mPickedUp = false;
    float mSwapTimer = 1.f;
};

} // namespace component
} // namespace snacgame
} // namespace ad
