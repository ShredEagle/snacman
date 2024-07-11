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


/// @brief Comput alpha from roughness parameter
float alphaFromRoughness(float aRoughness)
{
    // Disney PBR square the user provided roughness value (r) to obtain alpha.
    // hard to say if the metallic-roughness map already contains it squared, which would save instructions
    // (but would change the precision distribution)
    //
    // Also, from "Real Shading in Unreal Engine 4", some other massaging of roughness are possible.
    // For examplen to reduce "hotness" alpha = ((roughness + 1) / 2) ^ 2
    return aRoughness * aRoughness;
}


/// @param Xi (Chi) a random uniform sample on [0, 1].
/// @param N The surface normal in world space.
/// @return A half-vector (H) in world space that allows importance sampling of the GGX specular lobe.
vec3 importanceSampleGGX(vec2 Xi, float aAlphaSquared, vec3 N)
{
    float Phi = 2 * M_PI * Xi.x;
    // Inverse transform sampling (by cumulative distribution function inversion)
    // see: https://chatgpt.com/share/faad0830-d3b4-4c91-9abc-49a0cdce4dc6
    float CosTheta = sqrt((1 - Xi.y) / (1 + (aAlphaSquared - 1) * Xi.y));
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
vec3 specularIBL(vec3 SpecularColor, float aAlphaSquared, vec3 N, vec3 V, samplerCube aEnvMap)
{
    vec3 specularLighting = vec3(0);

    // Does not depend on the sample
    float NoV = dotPlus(N, V);
    if(NoV == 0) // Would cause a divisioh by zero on accumulation
    {
        return vec3(0.);
    }

    //const uint NumSamples = 1024;
    const uint NumSamples = 256;
    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley(i, NumSamples);
        vec3 H = importanceSampleGGX(Xi, aAlphaSquared, N);
        vec3 L = reflect(-V, H);
        float NoL = dotPlus(N, L);
        float NoH = dotPlus(N, H);
        float VoH = dotPlus(V, H);
        if(NoL > 0)
        {
            vec3 sampleColor = textureLod(aEnvMap, vec3(L.xy, -L.z), 0).rgb;
            float G = G2_GGX(NoL, NoV, aAlphaSquared);
            vec3 F = schlickFresnelReflectance(VoH, SpecularColor);
            // Incident light = sampleColor * NoL
            // Microfacet specular = D*G*F / (4*NoL*NoV)
            // pdf = D * NoH / (4 * VoH)
            specularLighting += sampleColor * F * G * VoH / (NoH * NoV);
        }
    }
    return specularLighting / NumSamples;
}


// Use the original environment mipmaps when prefiltering, which immensely helps with aliasing
#define PREFILTER_LEVERAGE_LOD

/// @param R the \b normalized reflection direction 
/// (i.e. the direciton that would be sampled in the filtered map)
vec3 prefilterEnvMap(float aAlphaSquared, vec3 R, samplerCube aEnvMap)
{
    const uint NumSamples = 1024;

    // To use any lobe, this approach imposes n = v = r
    // see: rtr 4th p421
    // This is an isotropic assumption, making the lobe radially symmetric.
    vec3 N = R;
    vec3 V = R;
    
#if defined(PREFILTER_LEVERAGE_LOD)
    ivec2 envSize = textureSize(aEnvMap, 0);
    float lodUniform = log2(envSize.x * envSize.y / NumSamples) / 2;
#endif

    // The accumulator
    vec3 prefilteredColor = vec3(0);
    float totalWeight = 0.f;
    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley(i, NumSamples);
        vec3 H = importanceSampleGGX(Xi, aAlphaSquared, N);
        vec3 L = reflect(-V, H);
        float nDotL = dotPlus(N, L);
        if(nDotL > 0)
        {
            vec3 sampleDir = vec3(L.xy, -L.z);
            
#if defined(PREFILTER_LEVERAGE_LOD)
            vec3 sampled;
            // Improtant: Distribution_GGX does not handle alpha == 0 (the theoretical result is +inf)
            // Anyway, we should probably always sample the mipmap level matching pixel coverage 
            // (only one sample direction should be possible when alpha == 0)
            if(aAlphaSquared != 0.)
            {
                // From "Real Shading in Unreal Engine 4": pdf = D * NoH / (4 * VoH)
                // From GPU Gems 3 chapter 20:
                // lod = max(0, 1/2 log2(w*h/N) - 1/2 log2(p(u,v) d(u)))

                float NoH = dot(N, H);
                // Note: NoH == VoH because of the isotropic assumtion (see above that N == V == R).
                // Note: The article was using dual parabolloid mapping (where distortion d(u) gets large)
                //       For cubemap, distortion is low, let's assume it is constant to 1 atm.
                // Note: Distribution_GGX returns the NDF multiplied by Pi (not sure we have to divide though)
                float D = Distribution_GGX(NoH, aAlphaSquared) / M_PI;
                float lod = max(0, lodUniform - (log2(D / 4) / 2));
                sampled = textureLod(aEnvMap, sampleDir, lod).rgb;

            #if 0 
                // Alternative implementation from https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/
                // which gives visually close results (I think it is equivalent up to a constant factor)
                float pdf = (D /** NdotH*/ / (4.0 /** HdotV*/)) + 0.0001; 
                float resolution = 2048.0; // resolution of source cubemap (per face)
                float saTexel  = 4.0 * M_PI / (6.0 * resolution * resolution);
                float saSample = 1.0 / (float(N) * pdf + 0.0001);
                float mipLevel = 0.5 * log2(saSample / saTexel); 

                sampled = textureLod(aEnvMap, sampleDir, lod).rgb;
            #endif
            }
            else
            {
                // The sample direction should be unique, with L == N == V == R.
                // i.e. we can leverage normal LODs calculation
                // Also, we should only ever take 1 sample...
                sampled = texture(aEnvMap, sampleDir).rgb;
            }
#else
            vec3 sampled = min(vec3(500.),
                               textureLod(aEnvMap, vec3(L.xy, -L.z), 0).rgb);
#endif

            prefilteredColor += sampled * nDotL;
            totalWeight += nDotL;
        }
    }

    return prefilteredColor / totalWeight;
}


#endif //include guard