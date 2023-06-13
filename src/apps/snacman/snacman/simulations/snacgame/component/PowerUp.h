#pragma once

#include <entity/Entity.h>

#include <math/Quaternion.h>
#include <math/Vector.h>

#include <array>
#include <variant>

namespace ad {
namespace snacgame {
namespace component {

enum class PowerUpType : unsigned int
{
    Dog,
    Teleport,
    Missile,
    None,
    _End = None,
};

struct PowerUpBaseInfo
{
    const char * mPath = "";
    const char * mPlayerPath = "";
    const char * mProgPath = "";
    const math::Quaternion<float> mLevelOrientation =
        math::Quaternion<float>::Identity();
    const math::Quaternion<float> mPlayerOrientation =
        math::Quaternion<float>::Identity();
    const math::Size<3, float> mLevelInstanceScale = {1.f, 1.f, 1.f};
    const math::Size<3, float> mPlayerInstanceScale = {1.f, 1.f, 1.f};
    const float mLevelScaling = 1.f;
    const float mPlayerScaling = 1.f;
    const math::Vec<3, float> mPosOffset = math::Vec<3, float>::Zero();
    const math::Vec<3, float> mPlayerPosOffset = math::Vec<3, float>::Zero();
};

constexpr float gMissileSpeed = 5.f;

const math::Quaternion<float> gLevelBasePowerupQuat =  math::Quaternion<float>{
    math::UnitVec<3, float>{{1.f, 0.f, 0.f}}, math::Turn<float>{0.2f}};

constexpr math::Quaternion<float> gBaseMissileOrientation = math::Quaternion<float>{0.f, 0.f, -0.707f, 0.707f};

constexpr std::array<PowerUpBaseInfo,
                     static_cast<unsigned int>(PowerUpType::_End)>
    gPowerupInfoByType{
        PowerUpBaseInfo{
            .mPath = "models/collar/collar.gltf",
            .mPlayerPath = "models/dog/dog.gltf",
            .mProgPath = "effects/MeshTextures.sefx",
            .mPlayerOrientation = math::Quaternion<float>{0.f, 0.707f, 0.f, 0.707f},
            .mLevelScaling = 0.2f,
            .mPlayerScaling = 0.03f,
            .mPlayerPosOffset = {0.f, -1.f, 0.f},
        },
        PowerUpBaseInfo{
            .mPath = "models/teleport/teleport.gltf",
            .mPlayerPath = "models/teleport/teleport.gltf",
            .mProgPath = "effects/MeshTextures.sefx",
            .mLevelScaling = 0.3f,
            .mPlayerScaling = 0.3f,
        },
        PowerUpBaseInfo{
            .mPath = "models/missile/missile.gltf",
            .mPlayerPath = "models/missile/missile.gltf",
            .mProgPath = "effects/MeshTextures.sefx",
            .mPlayerOrientation = gBaseMissileOrientation,
            .mPlayerInstanceScale = {0.3f, 0.3f, 0.3f},
            .mLevelScaling = 0.3f,
            .mPlayerScaling = 1.f
        }
    };

struct PowerUp
{
    PowerUpType mType;
    bool mPickedUp = false;
    float mSwapPeriod = 1.f;
    float mSwapTimer = mSwapPeriod;
};

struct InGameDog
{
};

constexpr float gMissileDelayFlashing = 4.f;
constexpr float gMissileDelayExplosion = 5.f;

struct InGameMissile
{
    ent::Handle<ent::Entity> mModel;
    ent::Handle<ent::Entity> mDamageArea;
    float mDelayExplosion = gMissileDelayExplosion;
};

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
    std::variant<InGameMissile, InGameDog> mInfo;
};

} // namespace component
} // namespace snacgame
} // namespace ad
