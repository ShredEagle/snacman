#version 420


#include "HelpersIbl.glsl"

in vec3 ex_FragmentPosition_world;

uniform samplerCube u_SkyboxTexture;

uniform float u_AlphaSquared;

layout(location = 0) out vec3 out_LinearHdr;


void main()
{
    out_LinearHdr = prefilterEnvMap(u_AlphaSquared, normalize(ex_FragmentPosition_world), u_SkyboxTexture);
}
