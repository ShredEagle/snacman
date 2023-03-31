#version 420

#include "Gamma.glsl"

in vec4 ex_Albedo;

out vec4 out_Color;

void main(void)
{
    out_Color = correctGamma(ex_Albedo);
}
