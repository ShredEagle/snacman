#pragma once

#include <renderer/GL_Loader.h>

#include <math/Vector.h>


namespace ad {
namespace bawls {
namespace component {


struct Velocity : public math::Vec<2, GLfloat>
{
    using math::Vec<2, GLfloat>::Vec;
};


} // namespace component
} // namespace bawls
} // namespace ad
