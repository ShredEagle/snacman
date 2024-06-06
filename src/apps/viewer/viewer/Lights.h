#pragma once


#include "runtime_reflect/ReflectHelpers.h"

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/GL_Loader.h>

#include <span>


namespace ad::renderer {


// TODO Ad 2024/06/05: How to keep that in sync with the shader code?
constexpr unsigned int gMaxLights = 16;

// TODO there are obvious ways to pack the values much more tightly

struct DirectionalLight
{
    // Needs to be default constructible
    alignas(16) math::UnitVec<3, GLfloat> mDirection{{0.f, 0.f, 1.f}};
    alignas(16) math::hdr::Rgb<GLfloat> mDiffuseColor = math::hdr::gWhite<GLfloat>;
    alignas(16) math::hdr::Rgb<GLfloat> mSpecularColor = math::hdr::gWhite<GLfloat>;
};


template <class T_visitor>
void r(T_visitor & aV, DirectionalLight & aLight)
{
    give(aV, aLight.mDirection, "direction");
    give(aV, aLight.mDiffuseColor, "diffuse color");
    give(aV, aLight.mSpecularColor, "specular color");
}


struct Radius
{
    // see rtr 4th p111
    // min is used as both
    // * r_0 (the distance at which the intensities are given)
    // * r_min (the distance below which light will not gain intensities anymore)
    // max is used as r_max, i.e. where both light intensity & derivative should reach zero.
    GLfloat mMin;
    GLfloat mMax;
};


template <class T_visitor>
void r(T_visitor & aV, Radius & aRadius)
{
    give(aV, aRadius.mMin, "min");
    give(aV, aRadius.mMax, "max");
}


struct PointLight
{
    alignas(16) math::Position<3, GLfloat> mPosition;
    // Important: the radius is how we control the "windowing" of the light falloff
    // in particular, minimum should not be too small, or the attenuation factor will very rapidly get close to 0.
    alignas(8) Radius mRadius; 
    alignas(16) math::hdr::Rgb<GLfloat> mDiffuseColor = math::hdr::gWhite<GLfloat>;
    alignas(16) math::hdr::Rgb<GLfloat> mSpecularColor = math::hdr::gWhite<GLfloat>;
};


template <class T_visitor>
void r(T_visitor & aV, PointLight & aLight)
{
    give(aV, aLight.mPosition, "position");
    give(aV, aLight.mRadius, "Radius");
    give(aV, aLight.mDiffuseColor, "diffuse color");
    give(aV, aLight.mSpecularColor, "specular color");
}


struct LightsData
{
    // see: https://registry.khronos.org/OpenGL/specs/gl/glspec45.core.pdf#page=159
    // "If the member is a scalar consuming N basic machine units, the base alignment is N.""
    /*alignas(16)*/ GLuint mDirectionalCount{0};
    /*alignas(16)*/ GLuint mPointCount{0};

    alignas(16) math::hdr::Rgb<GLfloat> mAmbientColor;
    std::array<DirectionalLight, gMaxLights> mDirectionalLights;
    std::array<PointLight, gMaxLights> mPointLights;
};


template <class T_visitor>
void r(T_visitor & aV, LightsData & aLights)
{
    give(aV, aLights.mAmbientColor, "ambient color");

    give(aV, Clamped<GLuint>{aLights.mDirectionalCount, 0, gMaxLights}, "directional count");
    give(aV, std::span{aLights.mDirectionalLights.data(), aLights.mDirectionalCount}, "directional lights");

    give(aV, Clamped<GLuint>{aLights.mPointCount, 0, gMaxLights}, "point count");
    give(aV, std::span{aLights.mPointLights.data(), aLights.mPointCount}, "point lights");
}


} // namespace ad::renderer