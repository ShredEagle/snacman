#version 420

#include "Helpers.glsl"

in vec3 ex_Normal_cam;

out vec4 out_Color;

void main()
{
    vec3 normal_cam = normalize(ex_Normal_cam);
    out_Color = vec4(remapToRgb(normal_cam, 1.), 1.);
}
