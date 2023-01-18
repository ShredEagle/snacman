#pragma once

#include "../component/Geometry.h"
#include "../EntityWrap.h"
#include "../GraphicState.h"

#include "../../../Input.h"

#include <entity/Query.h>
#include <snac-renderer/Camera.h>

#include <math/Constants.h>

namespace ad {
namespace snacgame {
namespace system {

class OrbitalCamera
{
  public:
    OrbitalCamera(ent::EntityManager & aWorld) :
        mCamera{aWorld, gInitialCameraSpherical.radius(), gInitialCameraSpherical.polar(),
                gInitialCameraSpherical.azimuthal()}
    {
    }

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    visu::Camera getCamera() const;

  private:
    static constexpr math::Spherical<float> gInitialCameraSpherical{
        75.f, math::Radian<float>{0.15f * math::pi<float>}, math::Radian<float>{0.5f * math::pi<float>}};
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1 / 500.f, 1 / 500.f};
    static constexpr float gScrollFactor = 0.05f;

    EntityWrap<snac::Orbital> mCamera;
};

} // namespace system
} // namespace snacgame
} // namespace ad
