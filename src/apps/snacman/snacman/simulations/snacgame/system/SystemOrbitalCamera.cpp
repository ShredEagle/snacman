#include "SystemOrbitalCamera.h"

#include "../GraphicState_V2.h"
#include "../GameParameters.h"

#include "../component/Geometry.h"

#include <entity/EntityManager.h>


namespace ad {
namespace snacgame {
namespace system {

OrbitalCamera::OrbitalCamera(ent::EntityManager & aWorld) :
    mControl{aWorld,
             OrbitalControlInput{
                .mOrbital = {
                    gInitialCameraSpherical.radius(),
                    gInitialCameraSpherical.polar(),
                    gInitialCameraSpherical.azimuthal()
                }
             }}
{}


void OrbitalCamera::update(const RawInput & aInput,
                           math::Radian<float> aVerticalFov,
                           int aWindowHeight_screen)
{
    mControl->update(aInput, aVerticalFov, aWindowHeight_screen);
}


visu_V1::Camera OrbitalCamera::getCamera() const
{
    return mControl->getCameraState();
}


} // namespace system
} // namespace cubes
} // namespace ad
