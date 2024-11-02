#pragma once


#include "runtime_reflect/ReflectHelpers.h"

#include <handy/array_utils.h>

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/GL_Loader.h>

#include <span>
#include <tuple>


namespace ad::renderer {


// TODO Ad 2024/06/05: How to keep that in sync with the shader code?
// TODO can we use the define system to forward those upper limit to shaders?
constexpr unsigned int gMaxLights = 16;
constexpr unsigned int gMaxShadowLights = 4;
constexpr unsigned int gCascadesPerShadow = 4;
constexpr unsigned int gMaxShadowMaps = gMaxShadowLights * gCascadesPerShadow;

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
    // In Lights.glsl, the colors are defined as vec4 (4 floats), so the alignment of the next member is 16 (4 * 4).
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


/// @brief Data that is user controlled, and part of the LightsBlock UBO
struct LightsDataUser
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


/// @brief SOA extension to LightsData_glsl, that is used for internal bookeeping (not user facing)
/// and part of the Lights UBO.
///
/// It maintains an array with an entry for each light, providing the base index of the shadow map
/// for associated light (this base should be offset by the cascade index for CSM).
struct LightsDataInternal
{
    // TODO Ad 2024/10/24: Use a compact approach instead of wasting 75% of this array in padding.
    // For an array of scalars, base alignment of the array, as well as array stride (i.e. individual members alignment) 
    // are set to the base alignment of the scalar (here GLuint -> 4) rounded up to base alignment of a vec4 (16).
    // Resulting alignment and stride is thus 16.
    // see: std140 storage layout, GL core spec 4.6, p146, item 4.
    struct alignas(16) IndexPadded
    {
        /*implicit*/ IndexPadded(Index aIdx) :
            mIndex{aIdx}
        {}

        /*implicit*/ operator Index & ()
        { return mIndex; }

        /*implicit*/ operator Index () const
        { return mIndex; }

        Index mIndex;
    };

    alignas(16) std::array<IndexPadded, gMaxLights> mDirectionalLightShadowMapIndices =
        make_filled_array<IndexPadded, gMaxLights>(IndexPadded{gNoEntryIndex});
};


/// @brief Concatenate all structures to be pushed as LightsBlock UBO.
struct LightsData_glsl : public LightsDataUser, LightsDataInternal
{};


/// @brief Extra control for the users, **not** part of the UBO.
struct ProjectShadowToggle
{
    /*implicit*/ operator bool() const
    { return mIsProjectingShadow; }

    bool mIsProjectingShadow{false};
};


template <class T_visitor>
void r(T_visitor & aV, ProjectShadowToggle & aToggle)
{
    give(aV, aToggle.mIsProjectingShadow, "project shadow");
}


/// @brief UI only struct, not to be loaded in UBO.
struct LightsDataToggleShadow
{
    std::array<ProjectShadowToggle, gMaxLights> mDirectionalLightProjectShadow;
};


/// @brief Concatenate all structures that are user facing.
struct LightsDataUi : public LightsDataUser, LightsDataToggleShadow
{
    std::span<ProjectShadowToggle> spanDirectionalLightProjectShadow()
    { return std::span{mDirectionalLightProjectShadow.data(), mDirectionalCount}; }
};


template <class T_visitor>
void r(T_visitor & aV, LightsDataUi & aLights)
{
    give(aV, aLights.mAmbientColor, "ambient color");

    give(aV, Clamped<GLuint>{aLights.mDirectionalCount, 0, gMaxLights}, "directional count");
    give(aV,
         std::make_tuple(aLights.spanDirectionalLights(), 
                         aLights.spanDirectionalLightProjectShadow()),
         "directional lights");

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