#pragma once 

#include "math/Quaternion.h"
#include "math/Vector.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <array>

namespace ad {
namespace snacgame {

inline constexpr int gTextSize = snac::gDefaultPixelHeight * 3;

inline constexpr const char * gDonutModel = "models/donut/donut.sel";

inline constexpr const char * gMeshGenericEffect = "effects/MeshOpaque.sefx";

inline constexpr const char * gEnvMap = "envmaps/christmas_photo_studio_05/split-christmas_photo_studio_05_8k.dds";


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

struct ModelInfo
{
    const char * mPath = "";
    const char * mProgPath = "";
    const math::Quaternion<float> mOrientation =
        math::Quaternion<float>::Identity();
    const math::Size<3, float> mInstanceScale = {1.f, 1.f, 1.f};
    const float mScaling = 1.f;
    const math::Vec<3, float> mPosOffset = math::Vec<3, float>::Zero();
};

const math::Quaternion<float> gLevelBasePowerupQuat =  math::Quaternion<float>{
    math::UnitVec<3, float>{{1.f, 0.f, 0.f}}, math::Turn<float>{0.2f}};

constexpr math::Quaternion<float> gBaseMissileOrientation = math::Quaternion<float>{0.f, 0.f, -0.707f, 0.707f};

constexpr std::array<ModelInfo,
                     static_cast<unsigned int>(component::PowerUpType::_End)>
    gLevelPowerupInfoByType{
        ModelInfo{
            .mPath = "models/collar/collar.sel",
            .mProgPath = gMeshGenericEffect,
            .mScaling = 0.2f,
        },
        ModelInfo{
            .mPath = "models/teleport/teleport.sel",
            .mProgPath = gMeshGenericEffect,
            .mScaling = 0.3f,
        },
        ModelInfo{.mPath = "models/missile/missile.sel",
                        .mProgPath = gMeshGenericEffect,
                        .mScaling = 0.3f,
                        }};

constexpr std::array<ModelInfo,
                     static_cast<unsigned int>(component::PowerUpType::_End)>
    gPlayerPowerupInfoByType{
        ModelInfo{
            .mPath = "models/bomb/Bomb.sel",
            .mProgPath = gMeshGenericEffect,
            .mOrientation =
                math::Quaternion<float>::Identity(),
            .mScaling = 1.f,
            .mPosOffset = {0.f, -1.f, 0.f},
        },
        ModelInfo{
            .mPath = "models/teleport/teleport.sel",
            .mProgPath = gMeshGenericEffect,
            .mScaling = 0.3f,
        },
        ModelInfo{.mPath = "models/missile/missile.sel",
                        .mProgPath = gMeshGenericEffect,
                        .mOrientation = gBaseMissileOrientation,
                        .mInstanceScale = {0.3f, 0.3f, 0.3f},
                        .mScaling = 1.f}};
constexpr float gNumbersBaseScale = 0.5f;
constexpr math::Vec<3, float> gNumbersPosOffset = {0.f, 2.f, 0.f};
constexpr std::array<ModelInfo,
          4>
      gSlotNumbers{
          ModelInfo{
              .mPath = "models/numbers/one.sel",
              .mProgPath = gMeshGenericEffect,
              .mScaling = gNumbersBaseScale,
              .mPosOffset = gNumbersPosOffset,
          },
          ModelInfo{
              .mPath = "models/numbers/two.sel",
              .mProgPath = gMeshGenericEffect,
              .mScaling = gNumbersBaseScale,
              .mPosOffset = gNumbersPosOffset,
          },
          ModelInfo{
              .mPath = "models/numbers/three.sel",
              .mProgPath = gMeshGenericEffect,
              .mScaling = gNumbersBaseScale,
              .mPosOffset = gNumbersPosOffset,
          },
          ModelInfo{
              .mPath = "models/numbers/four.sel",
              .mProgPath = gMeshGenericEffect,
              .mScaling = gNumbersBaseScale,
              .mPosOffset = gNumbersPosOffset,
          },
      };
} // namespace snacgame
} // namespace ad
