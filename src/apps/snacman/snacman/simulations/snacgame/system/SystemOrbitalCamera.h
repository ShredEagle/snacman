#pragma once

#include "../EntityWrap.h"
#include "../GraphicState.h"

#include <snacman/Input.h>

#include <entity/Query.h>
#include <math/Constants.h>
#include <snac-renderer-V1/Camera.h>

namespace ad {
namespace snacgame {
namespace component {
struct Geometry;
}
namespace system {

class OrbitalCamera
{
public:
    OrbitalCamera(ent::EntityManager & aWorld);

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    visu::Camera getCamera() const;

private:
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1 / 500.f,
                                                               1 / 500.f};
    static constexpr float gScrollFactor = 0.05f;

    EntityWrap<snac::Orbital> mCamera;
};

} // namespace system
} // namespace snacgame
} // namespace ad
