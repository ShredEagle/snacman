#pragma once


#include <math/Vector.h>


namespace ad::renderer {


struct ShadowCascadeBlock
{
    math::Vec<4, GLfloat> mCascadeFarPlaneDepths_view;
};


} // namespace ad::renderer