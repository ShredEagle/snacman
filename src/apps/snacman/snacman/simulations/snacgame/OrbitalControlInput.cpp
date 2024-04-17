#include "OrbitalControlInput.h"

#include "GraphicState_V2.h"

#include <snacman/Input.h>


namespace ad::snacgame {


void OrbitalControlInput::update(const RawInput & aInput,
                                 math::Radian<float> aVerticalFov,
                                 int aWindowHeight_screen)
{
    if(aInput.mMouse.get(MouseButton::Left))
    {
        // Orbiting
        mOrbital.incrementOrbitRadians(-aInput.mMouse.mCursorDisplacement.cwMul(gMouseControlFactor));              
    }
    else if(aInput.mMouse.get(MouseButton::Middle))
    {
        // Panning
        float viewedHeightOrbitPlane_world = 2 * tan(aVerticalFov / 2) * std::abs(mOrbital.radius());
        float factor = viewedHeightOrbitPlane_world / (float)aWindowHeight_screen;
        mOrbital.pan(aInput.mMouse.mCursorDisplacement * factor);              
    }

    // Mouse scroll
    float factor = (1 - aInput.mMouse.mScrollOffset.y() * gScrollFactor);
    mOrbital.radius() *= factor;
}


} // namespace ad::snacgame