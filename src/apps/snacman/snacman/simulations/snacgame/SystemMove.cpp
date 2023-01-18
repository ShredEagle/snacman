#include "SystemMove.h"

namespace ad {
namespace snacgame {
namespace system {


void Move::update(float aDelta)
{
    mMovables.each([aDelta](component::Geometry & aGeometry)
    {
        // Naive rotation about the Y axis.
        aGeometry.mYRotation +=  gAngularSpeed * aDelta;
    });
}


} // namespace system
} // namespace cubes
} // namespace ad
