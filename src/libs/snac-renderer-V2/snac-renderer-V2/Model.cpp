#include "Model.h"

#include <math/Transformations.h>
#include <math/Interpolation/Interpolation.h>
#include <math/Interpolation/QuaternionInterpolation.h>


namespace ad::renderer {


Pose::operator math::AffineMatrix<4, float> () const
{
    return math::trans3d::scaleUniform(mUniformScale) 
        * mOrientation.toRotationMatrix()
        * math::trans3d::translate(mPosition);
}


Pose interpolate(const Pose & aLeft, const Pose & aRight, float aInterpolant)
{
    return Pose{
        .mPosition     = math::lerp(aLeft.mPosition, aRight.mPosition, aInterpolant),
        .mUniformScale = math::lerp(aLeft.mUniformScale, aRight.mUniformScale, aInterpolant),
        .mOrientation  = math::slerp(aLeft.mOrientation, aRight.mOrientation, aInterpolant),
    };
}


} // namespace ad::renderer