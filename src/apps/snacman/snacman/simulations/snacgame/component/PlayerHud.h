#pragma once

#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <entity/Entity.h>

#include <math/Vector.h>

#include <array>


namespace ad {
namespace snacgame {
namespace component {

struct PlayerSlot;

const std::array<math::Position<2, float>, 4> gHudPositions{
    math::Position<2, float>{-0.95f, 0.5f},
    math::Position<2, float>{-0.95f, -0.5f},
    math::Position<2, float>{0.7f, 0.5f},
    math::Position<2, float>{0.7f, -0.5f},
};

const std::array<const char *, static_cast<unsigned int>(PowerUpType::_End)> gPowerUpName{
    "Donut Seeking dog",
    "Donut swapper",
    "Donut controlled missile"
};


struct PlayerHud
{
    int getScore() const;
    const PlayerSlot & getSlot() const;
    const char * getPowerUpName() const;

    ent::Handle<ent::Entity> mPlayer;
    // Note: It is not convenient to populate this at the moment the powerup is picked up
    // since the PlayerHud is not part of the Player anymore.
    //const char * mPowerUpName = "";
};


} // namespace component
} // namespace snacgame
} // namespace ad
