#pragma once

#include "../GameParameters.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include <entity/Entity.h>

#include <math/Vector.h>

#include <array>


namespace ad {
namespace snacgame {
namespace component {


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

// TODO Why is the rotation in the plane of the table around Z and not Y?
namespace {
    const auto axis = math::UnitVec<3, float>::MakeFromUnitLength({0.f, 0.f, 1.f});
}

const std::array<math::Quaternion<float>, 4> gHudOrientationsWorld{
    math::Quaternion<float>{axis, math::Degree<float>{ -5.f}},
    math::Quaternion<float>{axis, math::Degree<float>{ 20.f}},
    math::Quaternion<float>{axis, math::Degree<float>{  5.f}},
    math::Quaternion<float>{axis, math::Degree<float>{-20.f}},
};

const std::array<std::string, static_cast<unsigned int>(PowerUpType::None)> gPowerUpName{
    "Seeking dog",
    "Donut swapper",
    "Controlled missile"
};

std::string getPowerUpName(ent::Handle<ent::Entity> aPlayer);

struct PlayerHud
{
    ent::Handle<ent::Entity> mScoreText;
    ent::Handle<ent::Entity> mRoundText;
    ent::Handle<ent::Entity> mPowerupText;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mScoreText);
        archive & SERIAL_PARAM(mRoundText);
        archive & SERIAL_PARAM(mPowerupText);
    }
};

SNAC_SERIAL_REGISTER(PlayerHud)


} // namespace component
} // namespace snacgame
} // namespace ad
