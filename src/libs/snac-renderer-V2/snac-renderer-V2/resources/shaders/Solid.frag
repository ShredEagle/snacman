#version 420

#include "Gamma.glsl"

in vec4 ex_Color;

out vec4 out_Color;

void main(void)
{
    out_Color = correctGamma(ex_Color);
}
