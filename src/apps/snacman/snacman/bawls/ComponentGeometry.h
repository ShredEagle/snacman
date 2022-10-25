#pragma once

#include <renderer/GL_Loader.h>

#include <math/Vector.h>


namespace ad {
namespace bawls {
namespace component {


struct Geometry
{
    math::Position<2, GLfloat> position;
    float radius{1.f};

    float mass() const
    { return (float)std::pow(radius, 2); }
};


} // namespace component
} // namespace bawls
} // namespace ad
