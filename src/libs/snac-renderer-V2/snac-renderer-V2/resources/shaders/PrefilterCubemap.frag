#version 430


#include "HelpersIbl.glsl"


in vec3 ex_FragmentPosition_world;

uniform samplerCube u_SkyboxTexture;

uniform float u_Roughness;

layout(location = 0) out vec3 out_LinearHdr;


void main()
{
    float alphaSquared = pow(alphaFromRoughness(u_Roughness), 2);
    out_LinearHdr = prefilterEnvMap(alphaSquared, normalize(ex_FragmentPosition_world), u_SkyboxTexture);
}
