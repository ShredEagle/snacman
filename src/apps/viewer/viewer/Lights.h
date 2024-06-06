#pragma once

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/GL_Loader.h>


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
    r(aV, aLight.mDirection, "direction");
    r(aV, aLight.mDiffuseColor, "diffuse color");
    r(aV, aLight.mSpecularColor, "specular color");
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
void r(T_visitor & aV, Radius & aRadius, const char *)
{
    r(aV, aRadius.mMin, "min");
    r(aV, aRadius.mMax, "max");
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
    r(aV, aLight.mPosition, "position");
    r(aV, aLight.mRadius, "Radius");
    r(aV, aLight.mDiffuseColor, "diffuse color");
    r(aV, aLight.mSpecularColor, "specular color");
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


} // namespace ad::renderer