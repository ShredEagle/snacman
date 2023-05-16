#pragma once

#include "../GameParameters.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <entity/Entity.h>

#include <math/Vector.h>

#include <array>


namespace ad {
namespace snacgame {
namespace component {

struct PlayerSlot;

// Note: Unused since we moved to bills
const std::array<math::Position<2, float>, 4> gHudPositionsScreenspace{
    math::Position<2, float>{-0.95f, 0.5f},
    math::Position<2, float>{-0.95f, -0.5f},
    math::Position<2, float>{0.7f, 0.5f},
    math::Position<2, float>{0.7f, -0.5f},
};

const std::array<math::Position<3, float>, 4> gHudPositionsWorld{
    math::Position<3, float>{-6.5f, 10.f, gPlayerHeight},
    math::Position<3, float>{-4.0f,  1.f, gPlayerHeight},
    math::Position<3, float>{20.5f, 10.f, gPlayerHeight},
    math::Position<3, float>{18.5f,  1.f, gPlayerHeight},
};

namespace {
    const auto axis = math::UnitVec<3, float>::MakeFromUnitLength({0.f, 0.f, 1.f});
}

const std::array<math::Quaternion<float>, 4> gHudOrientationsWorld{
    math::Quaternion<float>{axis, math::Degree<float>{ -5.f}},
    math::Quaternion<float>{axis, math::Degree<float>{ 20.f}},
    math::Quaternion<float>{axis, math::Degree<float>{  5.f}},
    math::Quaternion<float>{axis, math::Degree<float>{-20.f}},
};

const std::array<const char *, static_cast<unsigned int>(PowerUpType::_End)> gPowerUpName{
    "Seeking dog",
    "Donut swapper",
    "Controlled missile"
};


struct PlayerHud
{
    int getScore() const;
    const PlayerSlot & getSlot() const;
    const char * getPowerUpName() const;

    ent::Handle<ent::Entity> mScoreText;
    ent::Handle<ent::Entity> mPowerupText;

    ent::Handle<ent::Entity> mPlayer;
    // Note: It is not convenient to populate this at the moment the powerup is picked up
    // since the PlayerHud is not part of the Player anymore.
    //const char * mPowerUpName = "";
};


} // namespace component
} // namespace snacgame
} // namespace ad
