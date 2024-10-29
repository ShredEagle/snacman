#pragma once

#include "entity/Wrap.h"
#include "../GraphicState.h"
#include "../OrbitalControlInput.h"

#include <snacman/Input.h>

#include <entity/Query.h>
#include <math/Constants.h>
#include <snac-renderer-V1/Camera.h>

namespace ad {


namespace renderer {

    class Camera;

} // namespace renderer


namespace snacgame {


namespace component {

    struct Geometry;

}


namespace system {

class OrbitalCamera
{
public:
    OrbitalCamera(ent::EntityManager & aWorld, 
                  float aAspectRatio);

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    renderer::Camera getCamera();
    OrbitalControlInput mControl;

private:
    renderer::Camera mCamera;
};

} // namespace system
} // namespace snacgame
} // namespace ad
