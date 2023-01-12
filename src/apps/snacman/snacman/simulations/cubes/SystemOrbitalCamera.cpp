#include "SystemOrbitalCamera.h"

namespace ad {
namespace cubes {
namespace system {


void OrbitalCamera::update(const RawInput & aInput,
                           math::Radian<float> aVerticalFov,
                           int aWindowHeight_screen)
{
    if(aInput.get(snac::MouseButton::Left))
    {
        // Orbiting
        mCamera->incrementOrbitRadians(-aInput.mCursorDisplacement.cwMul(gMouseControlFactor));              
    }
    else if(aInput.get(snac::MouseButton::Middle))
    {
        // Panning
        snac::Orbital & orbital = *mCamera;
        float viewedHeightOrbitPlane_world = 2 * tan(aVerticalFov / 2) * std::abs(orbital.radius());
        float factor = viewedHeightOrbitPlane_world / aWindowHeight_screen;
        orbital.pan(aInput.mCursorDisplacement * factor);              
    }

    // Mouse scroll
    float factor = (1 - aInput.mScrollOffset.y() * gScrollFactor);
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
