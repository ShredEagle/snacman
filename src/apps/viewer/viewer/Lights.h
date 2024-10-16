#pragma once


#include "runtime_reflect/ReflectHelpers.h"

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/GL_Loader.h>

#include <span>


namespace ad::renderer {


// TODO Ad 2024/06/05: How to keep that in sync with the shader code?
// TODO can we use the define system to forward those upper limit to shaders?
constexpr unsigned int gMaxLights = 16;
constexpr unsigned int gMaxShadowLights = 4;
constexpr unsigned int gMaxCascadesPerShadow = 4;
constexpr unsigned int gMaxShadowMaps = gMaxShadowLights * gMaxCascadesPerShadow;

// TODO there are obvious ways to pack the values much more tightly


using Index = GLuint;
inline constexpr Index gNoEntryIndex = std::numeric_limits<Index>::max();


struct DirectionalLight
{
    // Needs to be default constructible
    alignas(16) math::UnitVec<3, GLfloat> mDirection{{0.f, 0.f, 1.f}};
    alignas(16) math::hdr::Rgb<GLfloat> mDiffuseColor = math::hdr::gWhite<GLfloat>;
    alignas(16) math::hdr::Rgb<GLfloat> mSpecularColor = math::hdr::gWhite<GLfloat>;
    // alignment rules for std140 UB are defined by core glspec 7.6.2.2
    // In Lights.glsl, the colors are defined ad vec4 (4 floats), so the alignment of the next member is 16 (4 * 4).
    alignas(16) Index mShadowMapIndex = gNoEntryIndex;
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

    //
    // Helpers
    //
    std::span<DirectionalLight> spanDirectionalLights()
    { return std::span{mDirectionalLights.data(), mDirectionalCount}; }

    std::span<const DirectionalLight> spanDirectionalLights() const
    { return std::span{mDirectionalLights.data(), mDirectionalCount}; }

    std::span<PointLight> spanPointLights()
    { return std::span{mPointLights.data(), mPointCount}; }

    std::span<const PointLight> spanPointLights() const
    { return std::span{mPointLights.data(), mPointCount}; }
};


template <class T_visitor>
void r(T_visitor & aV, LightsData & aLights)
{
    give(aV, aLights.mAmbientColor, "ambient color");

    give(aV, Clamped<GLuint>{aLights.mDirectionalCount, 0, gMaxLights}, "directional count");
    give(aV, aLights.spanDirectionalLights(), "directional lights");

    give(aV, Clamped<GLuint>{aLights.mPointCount, 0, gMaxLights}, "point count");
    give(aV, aLights.spanPointLights(), "point lights");
}


/// @brief Layout compatible with shader's `LightViewProjectionBlock`
struct LightViewProjection
{
    GLuint mLightViewProjectionCount{0};
    // Note: this is a workaround for the fact that InstantiatedViewpoint GS hardcodes 4 invocations
    // (chosen to match the number of cascades for a single light).
    // This implies that for any light except the 1st, we will need to offset each invocation so they use the correct
    // view-projection matrix, and write the result to the correct layer.
    // TODO Ad 2024/10/10: #parameterize_shaders Somehow allowing to compile a program with the required number of invocations 
    // would allow to get rid of this member (and make a single call to draw all lights shadow maps at once)
    GLuint mLightViewProjectionOffset{0};
    // TODO can we use the define system to forward this upper limit to shaders?
    alignas(16) math::Matrix<4, 4, GLfloat> mLightViewProjections[gMaxShadowMaps];
};


} // namespace ad::renderer