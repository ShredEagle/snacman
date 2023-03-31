#version 420

#include "Gamma.glsl"

out vec4 out_Color;

void main(void)
{
    out_Color = correctGamma(vec4(1.f, 0.f, 1.f, 1.f));
}
