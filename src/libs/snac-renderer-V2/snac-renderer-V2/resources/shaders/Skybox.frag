#version 420

#include "Gamma.glsl"

in vec3 ex_CubeTextureCoords;

uniform samplerCube u_SkyboxTexture;

out vec4 out_Color;


void main()
{
    out_Color = correctGamma(texture(u_SkyboxTexture, ex_CubeTextureCoords));
}
