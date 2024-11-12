#include "SystemOrbitalCamera.h"

#include "../component/Geometry.h"
#include "../GameParameters.h"
#include "../GraphicState_V2.h"

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

OrbitalCamera::OrbitalCamera(ent::EntityManager & aWorld, float aAspectRatio) :
    mControl{.mOrbital = {gInitialCameraSpherical.radius(),
                          gInitialCameraSpherical.polar(),
                          gInitialCameraSpherical.azimuthal()}},
    mCamera{}
{
    const graphics::PerspectiveParameters initialPerspective{
        .mAspectRatio = aAspectRatio,
        .mVerticalFov = math::Degree<float>{45.f},
        .mNearZ = -0.01f,
        .mFarZ = -40.f,
    };
    mCamera.setupPerspectiveProjection(initialPerspective);
}

// TODO should be able to update aspect ratio when window is resized
void OrbitalCamera::update(const RawInput & aInput,
                           math::Radian<float> aVerticalFov,
                           int aWindowHeight_screen)
{
    mControl.update(aInput, aVerticalFov, aWindowHeight_screen);
}

renderer::Camera OrbitalCamera::getCamera()
{
    mCamera.setPose(mControl.mOrbital.getParentToLocal());
    return mCamera;
}

} // namespace system
} // namespace snacgame
} // namespace ad
