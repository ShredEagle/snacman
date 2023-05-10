#pragma once

#include "math/Vector.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <array>
namespace ad {
namespace snacgame {
namespace component {
const std::array<math::Position<2, float>, 4> gHudPositions{
    math::Position<2, float>{-0.95f, 0.5f},
    math::Position<2, float>{-0.95f, -0.5f},
    math::Position<2, float>{0.7f, 0.5f},
    math::Position<2, float>{0.7f, -0.5f},
};

const std::array<const char *, static_cast<unsigned int>(PowerUpType::_End)> gPowerUpName{
    "Donut Seeking dog",
    "Donut swapper",
};

struct PlayerHud
{
    unsigned int mScore;
    const char * mPowerUpName = "";
};
} // namespace component
} // namespace snacgame
} // namespace ad
