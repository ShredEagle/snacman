#pragma once

#include <math/Angle.h>

#include <snac-renderer-V2/Camera.h>

namespace ad {
    struct RawInput;
} // namespace ad

namespace ad::snacgame {


namespace visu {
    struct Camera;
} // namespace visu

/// @brief Controls an orbital from RawInput
struct OrbitalControlInput
{
    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);
    

    visu::Camera getCameraState();

    renderer::Orbital mOrbital;

private:
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1 / 500.f,
                                                               1 / 500.f};
    static constexpr float gScrollFactor = 0.05f;
};


} // namespace ad::snacgame