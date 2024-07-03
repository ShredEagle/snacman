//
// See: Real Shading in Unreal Engine 4, Brian Karis
//

#if !defined(HELPERS_IBL_GLSL_INCLUDE_GUARD)
#define HELPERS_IBL_GLSL_INCLUDE_GUARD


#include "Helpers.glsl"
#include "HelpersPbr.glsl"


//////
// see: https://learnopengl.com/PBR/IBL/Specular-IBL
//////

float radicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


/// @brief Provide the Hammersley Sequence 
/// A random low-discrepency sequence usefull for importance sampling
/// in quasi Monte Carlo integration method.
vec2 hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}  


//////
//
//////


/// @param Xi (Chi) a random uniform sample on [0, 1].
/// @param N The surface normal in world space.
/// @return A half-vector (H) in world space that allows importance sampling of the GGX specular lobe.
vec3 importanceSampleGGX(vec2 Xi, float aRoughness, vec3 N)
{
    // TODO: is a "alpha" (I guess so), or is it more alpha_squared (in which case aRoughness should actually be alpha)
    float a = aRoughness * aRoughness;
    float Phi = 2 * M_PI * Xi.x;
    // Inverse transform sampling (by cumulative distribution function inversion)
    // see: https://chatgpt.com/share/faad0830-d3b4-4c91-9abc-49a0cdce4dc6
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a*a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    vec3 H;
    // H is the cartesian coordinates of half vector in tangent space.
    // (This is the spherical-to-cartesion from angles relative to the implicit normal and tangent)
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    // Compute the world coordinates of the tagent space basis vectors
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize( cross( UpVector, N ) );
    vec3 TangentY = cross( N, TangentX );
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}


/// @brief Specular contribution of Image Based Lighting from a cube environment map.
/// 
/// Computed via quasi Monte Carlo importance sampling of the environment map.
vec3 specularIBL(vec3 SpecularColor, float aRoughness, vec3 N, vec3 V, samplerCube aEnvMap)
{
    vec3 specularLighting = vec3(0);
    // Does not depend on the sample
    float NoV = dotPlus(N, V);
    if(NoV == 0)
    {
        // Would cause a divisioh by zero on accumulation
        return vec3(0.);
    }
    //const uint NumSamples = 1024;
    const uint NumSamples = 128;
    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley(i, NumSamples);
        vec3 H = importanceSampleGGX(Xi, aRoughness, N);
        vec3 L = 2 * dot(V, H) * H - V;
        float NoL = dotPlus(N, L);
        float NoH = dotPlus(N, H);
        float VoH = dotPlus(V, H);
        if(NoL > 0)
        {
            vec3 sampleColor = textureLod(aEnvMap, vec3(L.xy, -L.z), 0).rgb;
            /// TODO directly take alpha squared as parameter
            // Going from aRoughness (expected to be the artist-facing parameter)
            // to the expected alpha squared:
            float alphaSquared = (aRoughness * aRoughness);
            float G = G2_GGX(NoL, NoV, alphaSquared);
            // I do not understand this Fresnel, let's test with it
            float Fc = pow(1 - VoH, 5);
            vec3 F = (1 - Fc) * SpecularColor + Fc;
            // Incident light = sampleColor * NoL
            // Microfacet specular = D*G*F / (4*NoL*NoV)
            // pdf = D * NoH / (4 * VoH)
            specularLighting += sampleColor * F * G * VoH / (NoH * NoV);
        }
    }
    return specularLighting / NumSamples;
}


#endif //include guard