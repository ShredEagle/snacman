#pragma once
#include "math/Quaternion.h"
#include "math/Vector.h"

#include <array>
namespace ad {
namespace snacgame {
namespace component {

enum class PowerUpType : unsigned int
{
    Dog,
    Teleport,
    Missile,
    _End,
};

struct PowerUpBaseInfo
{
    const char * mPath = "";
    const math::Quaternion<float> mOrientation =
        math::Quaternion<float>::Identity();
    const math::Size<3, float> mInstanceScale = {1.f, 1.f, 1.f};
    const float mScaling = 1.f;
    const math::Vec<3, float> mPosOffset = math::Vec<3, float>::Zero();
};

const math::Quaternion<float> gBasePowerupQuat =  math::Quaternion<float>{
    math::UnitVec<3, float>{{1.f, 0.f, 0.f}}, math::Turn<float>{0.2f}};

constexpr std::array<PowerUpBaseInfo,
                     static_cast<unsigned int>(PowerUpType::_End)>
    gPowerupPathByType{
        PowerUpBaseInfo{
            .mPath = "models/collar/collar.gltf",
            .mScaling = 0.2f,
        },
        PowerUpBaseInfo{
            .mPath = "models/teleport/teleport.gltf",
            .mScaling = 0.3f,
        },
        PowerUpBaseInfo{
            .mPath = "models/missile/missile.gltf",
            .mScaling = 0.2f,
        },
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
