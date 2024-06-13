#include "Helpers.glsl"

vec3 schlickFresnelReflectance(vec3 aNormal, vec3 aLightDir, vec3 F0, vec3 F90 = vec3(1., 1., 1.))
{
    return F0 + (F90 - F0) * pow(1 - dotPlus(aNormal, aLightDir), 5);
}

float heaviside(float aValue)
{
    return float(aValue > 0.);
}

//
// Expanded versions
// (It seems the glTF viewer implementation is aware of some shortcuts).
// Note: The shortcut version seem to have a slightly stronger "Fresnel effect",
//       so it is not really equivalent.
//

// Important: all dot product are expected clamped to [0, 1]
float Distribution_GGX(float aRoughnessSquared, float nDotH)
{
    float n = aRoughnessSquared * heaviside(nDotH);

    float f = nDotH * nDotH * (aRoughnessSquared - 1) + 1;
    float d = M_PI * f * f;

    return n/d;
}


float _Visibility_GGX_denominator(float aRoughnessSquared, float plusDot)
{
    return plusDot + sqrt(aRoughnessSquared + (1 - aRoughnessSquared) * plusDot * plusDot);
}


float Visibility_GGX(float aRoughnessSquared, float hDotL, float hDotV, float nDotL, float nDotV)
{
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
    return alphaRoughnessSq / (M_PI * f * f);
}