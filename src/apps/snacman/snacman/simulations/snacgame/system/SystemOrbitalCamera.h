#pragma once

#include "entity/Wrap.h"
#include "imguiui/ImguiUi.h"
#include "reflexion/NameValuePair.h"
#include "snac-reflexion/Reflexion.h"
#include "../GraphicState.h"
#include "../OrbitalControlInput.h"

#include <snacman/Input.h>

#include <entity/Query.h>
#include <math/Constants.h>
#include <snac-renderer-V1/Camera.h>
#include <variant>

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
    OrbitalCamera() = default;
    OrbitalCamera(ent::EntityManager & aWorld, 
                  float aAspectRatio);

    void update(const RawInput & aInput,
                math::Radian<float> aVerticalFov,
                int aWindowHeight_screen);

    renderer::Camera getCamera();
    OrbitalControlInput mControl;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mControl.mOrbital.mSpherical));
        aWitness.witness(NVP(mControl.mOrbital.mSphericalOrigin));
    }

private:
    renderer::Camera mCamera;
};

REFLEXION_REGISTER(OrbitalCamera)

} // namespace system
} // namespace snacgame
} // namespace ad
