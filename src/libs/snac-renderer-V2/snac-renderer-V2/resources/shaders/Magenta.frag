#version 420

#include "Gamma.glsl"
 
out vec4 out_Color;

void main()
{
    // gamma correction does change anything for full intensity magenta
    out_Color = correctGamma(vec4(1., 0., 1., 1.));
}