#include "Helpers.glsl"


vec3 schlickFresnelReflectance(float aNormalDotLightPlus, vec3 F0, vec3 F90 = vec3(1., 1., 1.))
{
    return F0 + (F90 - F0) * pow(1 - aNormalDotLightPlus, 5);
}


vec3 schlickFresnelReflectance(vec3 aNormal, vec3 aLightDir, vec3 F0, vec3 F90 = vec3(1., 1., 1.))
{
    return schlickFresnelReflectance(dotPlus(aNormal, aLightDir), F0, F90);
}


float heaviside(float aValue)
{
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


// Important: returned multiplied by Pi, so the overall GGX specular BRDF is mutiplied by Pi
float Distribution_GGX(float nDotH, float aAlphaSq)
{
    float d = 1 + nDotH * nDotH * (aAlphaSq - 1);
    return (heaviside(nDotH) * aAlphaSq) / (d * d);
}


// Visibility is G2 / (4 |n.l| |n.v|)
// This is the simplified form of the height-correlated Smith G2 for GGX
// see rtr 4th p341
float Visibility_GGX(float nDotL, float nDotV, float aAlphaSq)
{
    float uo = nDotV;
    float ui = nDotL;
    float l = uo + sqrt(aAlphaSq + ui * (ui - aAlphaSq * ui));
    float r = ui + sqrt(aAlphaSq + uo * (uo - aAlphaSq * uo));
    // TODO is there a risk of divide by zero?
    return 0.5 / (l + r);
}


// Important: All BRDFs are given already mutiplied by Pi
// (because the correct formulas usually have a Pi in the denominator)
vec3 specularBrdf_GGX(vec3 aFresnelReflectance, 
                        float nDotH, float nDotL, float nDotV,
                        float aAlpha)
{
    float alphaSquared = aAlpha * aAlpha;
    float D = Distribution_GGX(nDotH, alphaSquared);
    float V = Visibility_GGX(nDotL, nDotV, alphaSquared);
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
    float d = M_PI * f * f;

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