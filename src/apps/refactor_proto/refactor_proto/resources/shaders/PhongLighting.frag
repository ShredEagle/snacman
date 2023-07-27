#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"
 
in vec3 ex_Position_cam;
in vec3 ex_Normal_cam;

out vec4 out_Color;


void main()
{
    // TODO take that as input
    vec3 lightColor = vec3(1., 1., 1.);
    vec3 lightDir_cam = normalize(vec3(-0.5, 0., 1.));
    vec3 ambientFactor = vec3(.2, .2, .2);
    vec4 albedo = vec4(1., 1., 1., 1.);

    vec3 view_cam = vec3(0., 0., 1.);
    vec3 h_cam = normalize(view_cam + lightDir_cam);
    vec3 normal_cam = normalize(ex_Normal_cam);

    vec3 ambient = lightColor * ambientFactor;
    vec3 diffuse = lightColor * dotPlus(normal_cam, lightDir_cam);

    vec3 phongColor = (ambient + diffuse) * albedo.xyz;
    out_Color = correctGamma(vec4(phongColor, albedo.w));
    //out_Color = vec4(normal_cam, 1.);
}