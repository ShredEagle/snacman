#include "Model.h"

#include <math/Transformations.h>


namespace ad::renderer {


Pose::operator math::AffineMatrix<4, float> () const
{
    return math::trans3d::scaleUniform(mUniformScale) * math::trans3d::translate(mPosition);
}


} // namespace ad::renderer