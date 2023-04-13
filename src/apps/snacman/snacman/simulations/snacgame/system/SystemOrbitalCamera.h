#pragma once

#include "../component/Geometry.h"
#include "../EntityWrap.h"
#include "../GraphicState.h"

#include <snacman/Input.h>

#include <entity/Query.h>
#include <math/Constants.h>
#include <snac-renderer/Camera.h>

namespace ad {
namespace snacgame {
namespace system {

class OrbitalCamera
{
public:
    OrbitalCamera(ent::EntityManager & aWorld) :
        mCamera{aWorld, gInitialCameraSpherical.radius(),
                gInitialCameraSpherical.polar(),
                gInitialCameraSpherical.azimuthal()}
    {}

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    visu::Camera getCamera() const;

private:
    static constexpr math::Spherical<float> gInitialCameraSpherical{
        20.f, math::Turn<float>{0.075f},
        math::Turn<float>{0.f}};
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1 / 500.f,
                                                               1 / 500.f};
    static constexpr float gScrollFactor = 0.05f;

    EntityWrap<snac::Orbital> mCamera;
};

} // namespace system
} // namespace snacgame
} // namespace ad
