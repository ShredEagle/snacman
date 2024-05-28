#version 420

in vec3 ex_Normal_cam;

out vec4 out_Color;

void main()
{
    vec3 normal_cam = normalize(ex_Normal_cam);
    out_Color = vec4((normal_cam + vec3(1.)) / 2, 1.);
}