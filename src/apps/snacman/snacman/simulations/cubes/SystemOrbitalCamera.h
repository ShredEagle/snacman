#pragma once

#include "ComponentGeometry.h"
#include "EntityWrap.h"
#include "GraphicState.h"

#include "../../Input.h"

#include <entity/Query.h>

#include <snac-renderer/Camera.h>


namespace ad {
namespace cubes {
namespace system {

class OrbitalCamera
{
public:
    OrbitalCamera(ent::EntityManager & aWorld) :
        mCamera{aWorld, gInitialCameraRadius}
    {}

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    visu::Camera getCamera() const; 


private:
    static constexpr float gInitialCameraRadius{6.f};
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1/500.f, 1/500.f};
    static constexpr float gScrollFactor = 0.05f;

    EntityWrap<snac::Orbital> mCamera;
};

} // namespace system
} // namespace cubes
} // namespace ad
