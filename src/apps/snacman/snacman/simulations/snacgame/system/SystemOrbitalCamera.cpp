#include "SystemOrbitalCamera.h"

#include "../component/Geometry.h"

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {
namespace system {

OrbitalCamera::OrbitalCamera(ent::EntityManager & aWorld) :
    mCamera{aWorld, gInitialCameraSpherical.radius(),
            gInitialCameraSpherical.polar(),
            gInitialCameraSpherical.azimuthal()}
{}


void OrbitalCamera::update(const RawInput & aInput,
                           math::Radian<float> aVerticalFov,
                           int aWindowHeight_screen)
{
    if(aInput.mMouse.get(MouseButton::Left))
    {
        // Orbiting
        mCamera->incrementOrbitRadians(-aInput.mMouse.mCursorDisplacement.cwMul(gMouseControlFactor));              
    }
    else if(aInput.mMouse.get(MouseButton::Middle))
    {
        // Panning
        snac::Orbital & orbital = *mCamera;
        float viewedHeightOrbitPlane_world = 2 * tan(aVerticalFov / 2) * std::abs(orbital.radius());
        float factor = viewedHeightOrbitPlane_world / (float)aWindowHeight_screen;
        orbital.pan(aInput.mMouse.mCursorDisplacement * factor);              
    }

    // Mouse scroll
    float factor = (1 - aInput.mMouse.mScrollOffset.y() * gScrollFactor);
    mCamera->radius() *= factor;
}


visu::Camera OrbitalCamera::getCamera() const
{
    return {
        .mWorldToCamera = mCamera->getParentToLocal(),
    };
}


} // namespace system
} // namespace cubes
} // namespace ad
