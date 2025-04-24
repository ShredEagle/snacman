#if !defined(HELPERS_PBR_GLSL_INCLUDE_GUARD)
#define HELPERS_PBR_GLSL_INCLUDE_GUARD


#include "Helpers.glsl"

const vec3 gF0_dielec = vec3(0.04);

vec3 schlickFresnelReflectance(float aNormalDotLight_plus, vec3 F0, vec3 F90)
{
    return F0 + (F90 - F0) * pow(1 - aNormalDotLight_plus, 5);
}


vec3 schlickFresnelReflectance(vec3 aNormal, vec3 aLightDir, vec3 F0, vec3 F90)
{
    return schlickFresnelReflectance(dotPlus(aNormal, aLightDir), F0, F90);
}


float heaviside(float aValue)
{
    // Note: I do not think we can implement it as step(0., aValue)
    // since it would return 1 when aValue is 0.
    return float(aValue > 0.);
}


// Important: All BRDFs are given already mutiplied by Pi
// (because the correct formulas usually have a Pi in the denominator)
// This is the Lambertian weighted by (1-F) as to represent the energy trade-off
// see rtr 4th p351
vec3 diffuseBrdf_weightedLambertian(vec3 aFresnelReflectance, vec3 aDiffuseColor)
{
    return (1 - aFresnelReflectance) * aDiffuseColor;
}


//
// Trowbridge-Reitz / GGX microfacet model
//

// Important: returned multiplied by Pi, so the overall GGX specular BRDF is mutiplied by Pi
/// @param aAlphaSq cannot be null
float Distribution_GGX(float nDotH, float aAlphaSq)
{
    // Note: I do not know how to handle alpha == 0:
    // the function becomes an asymptote where result is 0 but tends to +inf as nDotH tends to 1.

    float d = 1 + nDotH * nDotH * (aAlphaSq - 1);
    // TODO is heaviside useful here?
    return (heaviside(nDotH) * aAlphaSq) / (d * d);
}


/// @param aDot either vDotL or lDotN 
float Lambda_GGX(float aDot, float aAlphaSq)
{
    float dotSq = aDot * aDot;
    // rtr 4th p339 (9.37)
    float aSq = dotSq / (aAlphaSq * (1 - dotSq));
    // rtr 4th p341 (9.42)
    return (-1. + sqrt(1. + (1. / aSq))) / 2.;
}

/// @brief Smith height-correlated masking-shadowing function
/// @see rtr 4th p335
float G2_GGX(float nDotL, float nDotV, float aAlphaSq)
{
    // Our equivalent to heaviside (not sure how relevant it is)
    if ((nDotV == 0) || (nDotL == 0))
    {
        return 0.f;
    }
    else
    {
        float d = 1 + Lambda_GGX(nDotV, aAlphaSq) + Lambda_GGX(nDotL, aAlphaSq);
        return 1.f / d;
    }
}


// Visibility is how we call the the compound term: G2 / (4 |n.l| |n.v|)
/// @brief The explicit longform of the visibility (please use the a simplification/approximation for prod)
/// It is mainly intended to test the G2_GGX.
float Visibility_GGX_longform(float nDotL, float nDotV, float aAlphaSq)
{
    // Guard against division by zero (redundant with heaviside in G2_GGX)
    if ((nDotV == 0) || (nDotL == 0))
    {
        return 0;
    }
    else
    {
        return G2_GGX(nDotL, nDotV, aAlphaSq) / (4 * nDotL * nDotV);
    }
}


// Visibility is how we call the the compound term: G2 / (4 |n.l| |n.v|)
// TODO: Find the correct name, "visibility" might be an abuse.
//       Some texts seem to use "visibility" for the masking-shadowing function (G2).
/// @note This is the simplified (yet I assume exact) form of the height-correlated Smith G2 for GGX
/// see rtr 4th p341
float Visibility_GGX(float nDotL, float nDotV, float aAlphaSq)
{
    float uo = nDotV;
    float ui = nDotL;
    float l = uo * sqrt(aAlphaSq + ui * (ui - aAlphaSq * ui));
    float r = ui * sqrt(aAlphaSq + uo * (uo - aAlphaSq * uo));
    // I suppose there is a risk of division by zero
    // because I sometime get dark outlines if not making the test
    float d = l + r;
    if (d > 0)
    {
        return 0.5 / d;
    }
    else
    {
        return 0;
    }
}

// Hammon-Karis approximation of Trowbridge-Reitz/GGX visibility
// see: rtr 4th p342
float Visibility_GGX_approx(float nDotL, float nDotV, float aAlpha)
{
    // The approximation suffer from the same problem of division by zero
    // so we test the upper bound
    if ((nDotL + nDotV) > 0)
    {
        return 0.5 / mix(2 * nDotL * nDotV, nDotL + nDotV, aAlpha);
    }
    else
    {
        return 0;
    }
}

//#define APPROXIMATE_G2_GGX
#define SIMPLIFIED_VISIBILITY_GGX
//#define EXPLICIT_G2_GGX

// Important: All BRDFs are given already mutiplied by Pi
// (because the correct formulas usually have a Pi in the denominator)
vec3 specularBrdf_GGX(vec3 aFresnelReflectance, 
                      float nDotH, float nDotL, float nDotV,
                      float aAlpha)
{
    float alphaSquared = aAlpha * aAlpha;
    float D = Distribution_GGX(nDotH, alphaSquared);
#ifdef APPROXIMATE_G2_GGX
    float V = Visibility_GGX_approx(nDotL, nDotV, aAlpha);
#elif defined(SIMPLIFIED_VISIBILITY_GGX)
    float V = Visibility_GGX(nDotL, nDotV, alphaSquared);
#elif defined(EXPLICIT_G2_GGX)
    float V = Visibility_GGX_longform(nDotL, nDotV, alphaSquared);
#endif
    return aFresnelReflectance * V * D;
}


//
// Beckmann & Blinn-Phong models
//

/// @param aAlpha_beckmann Alpha value according to Bekcmann model.
/// **attention**: must not be zero, has to be strictly positive.
float alphaBeckmannToPhong(float aAlpha_beckmann)
{
    return 2 * pow(aAlpha_beckmann, -2) - 2;
}


// Important: All BRDFs are given already mutiplied by Pi
/// @param nDotH_plus dotPlus of N and H.
/// @note see rtr 4th p340
float Distribution_BlinnPhong(float nDotH_plus, float aAlpha_phong)
{
    // Note: The formula in rtr 4th p340 use the raw dot product, but we use dotPlus.
    // Rationale: GLSL pow() is undefined for x < 0., and this should be equivalent since
    // the heaviside operator in the formula means any negative nDotH should return 0,
    // which is also the case when nDotH is 0.
    // Note: we probably do not need heaviside, since 0^y is 0 (for any non-nul y).
    return /*heaviside(nDotH_plus) * */((aAlpha_phong + 2) / 2) * pow(nDotH_plus, aAlpha_phong);
}


/// @note The exact Lambda is known but considered too expensive for real-time.
/// see rtr 4th p339
/// @param aDot nDotL or nDotV, unclamped and signed.
/// @param aAlpha_beckmann should be strictly positive, **cannot** be null.
float Lambda_Beckmann_approximate(float aDot, float aAlpha_beckmann)
{
    float a = aDot / (aAlpha_beckmann * sqrt(1 - (aDot * aDot)));
    if (a < 1.6)
    {
        return (1 - 1.259 * a + 0.396 * a * a)
                / (3.535 * a + 2.181 * a * a);
    }
    else
    {
        return 0;
    }
}


/// @param nDotL is the **unclamped** signed dot product ("raw" dot product).
/// @param nDotV is the **unclamped** signed dot product ("raw" dot product).
float Visibility_Beckmann(float nDotL, float nDotV, float aAlpha_beckmann)
{
    float lambda_v = Lambda_Beckmann_approximate(nDotV, aAlpha_beckmann);
    float lambda_l = Lambda_Beckmann_approximate(nDotL, aAlpha_beckmann);
    float G2 = 1 / (1 + lambda_v + lambda_l);

    return G2 / (4 * abs(nDotL) * abs(nDotV));
}


/// @brief Overload taking the unit vectors directly, so there is no client-error
/// regarding dot products post-op.
float Visibility_Beckmann(vec3 n, vec3 l, vec3 v, float aAlpha_beckmann)
{
    return Visibility_Beckmann(dot(n, l), dot(n, v), aAlpha_beckmann);
}


/// @important aAlpha_beckmann cannot be zero, as it create a lot of numerical issues
/// (for the moment, this function ensures it is not, which has a runtime cost)
vec3 specularBrdf_BlinnPhong(vec3 aFresnelReflectance, 
                             float nDotH_plus, float nDotL_raw, float nDotV_raw,
                             float aAlpha_beckmann)
{
    // It is mandatory that alpha_beckmann is not null 
    // otherwise the conversion returns inf
    // (which is theoretically correct, but numerically problematic)
    // and the visibility returns nan.
    float alpha_b_safe = max(0.0001, aAlpha_beckmann);
    float D = Distribution_BlinnPhong(nDotH_plus, alphaBeckmannToPhong(alpha_b_safe));
    // rtr 4th p340 suggests using the Beckmann Lambda (thus, visibility).
    float V = Visibility_Beckmann(nDotL_raw, nDotV_raw, alpha_b_safe);
    return aFresnelReflectance * V * D;
}


//////////////////
// glTF versions
//////////////////

// Expanded versions
// (It seems the glTF viewer implementation is aware of some shortcuts).
// Note: The shortcut version seem to have a slightly stronger "Fresnel effect",
//       so it is not really equivalent.
//

// Important: all dot product are expected clamped to [0, 1]
float Distribution_GGX_gltf(float aRoughnessSquared, float nDotH)
{
    float n = aRoughnessSquared * heaviside(nDotH);

    float f = nDotH * nDotH * (aRoughnessSquared - 1) + 1;
    //float d = M_PI * f * f;
    float d = f * f;

    return n/d;
}


float _Visibility_GGX_denominator(float aRoughnessSquared, float plusDot)
{
    return plusDot + sqrt(aRoughnessSquared + (1 - aRoughnessSquared) * plusDot * plusDot);
}

// I think this is the separable form of the joint masking-shadowing function
// see rtr 4th p335
float Visibility_GGX_gltf(float aRoughnessSquared, float hDotL, float hDotV, float nDotL, float nDotV)
{
    // For some reason, a null roughness lead to inf in l or r
    // So setting a minimum value > 0 fixes this
    //aRoughnessSquared = max(0.0001, aRoughnessSquared);
    float l = heaviside(hDotL) / _Visibility_GGX_denominator(aRoughnessSquared, nDotL);
    float r = heaviside(hDotV) / _Visibility_GGX_denominator(aRoughnessSquared, nDotV);
    return l * r;
}


//
// Shortcut version from glTF viewer implementation
// see: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/v1.0.10/source/Renderer/shaders/brdf.glsl#L69-L98)
//

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX_gltf(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}


// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX_gltf(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    //return alphaRoughnessSq / (M_PI * f * f);
    return alphaRoughnessSq / (f * f);
}


#endif //include guard
