#version 420

#include "Gamma.glsl"
 
in vec2 ex_Uv;
in vec4 ex_ColorFactor;

out vec4 out_Color;

uniform sampler2DArray u_DiffuseTexture;

void main()
{
    out_Color = correctGamma(texture(u_DiffuseTexture, vec3(ex_Uv, 0)) * ex_ColorFactor);
}