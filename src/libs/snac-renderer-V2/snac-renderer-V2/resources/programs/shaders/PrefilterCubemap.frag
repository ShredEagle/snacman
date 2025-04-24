#version 430


#include "HelpersIbl.glsl"
#include "HelpersPbr.glsl"


in vec3 ex_FragmentPosition_world;

uniform samplerCube u_EnvironmentTexture;

uniform float u_Roughness;

layout(location = 0) out vec3 out_LinearHdr;


void main()
{
    float alphaSquared = pow(alphaFromRoughness(u_Roughness), 2);
    out_LinearHdr = 
#if defined(SPECULAR_RADIANCE)
        prefilterEnvMapSpecular(alphaSquared, normalize(ex_FragmentPosition_world), u_EnvironmentTexture);
#elif defined(DIFFUSE_IRRADIANCE)
        // Note: we use the F0 of dielectric, because metals do not have a diffuse contribution
        prefilterEnvMapDiffuse_LambertianFresnel(normalize(ex_FragmentPosition_world),
                                                 gF0_dielec,
                                                 u_EnvironmentTexture);
#endif
}
