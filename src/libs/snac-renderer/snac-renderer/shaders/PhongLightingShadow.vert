#version 420

layout(location=0) in vec3 ve_Position_l;
layout(location=1) in vec3 ve_Normal_l;

layout(location=4) in mat4 in_LocalToWorld;
// Will be required to support non-uniform scaling.
//layout(location=8) in mat4 in_LocalToWorldInverseTranspose;
layout(location=12) in vec4 in_Albedo;

layout(std140) uniform ViewingBlock
{
    mat4 u_WorldToCamera;
    mat4 u_Projection;
};

uniform mat4 u_LightViewingMatrix;

out vec4 ex_Position_v;
out vec4 ex_Normal_v;
out vec4 ex_Position_lightClip;
out vec3 ex_DiffuseColor;
out vec3 ex_SpecularColor;
out float ex_Opacity;

void main(void)
{
    // TODO maybe should be pre-multiplied in client code?
    mat4 localToCamera = u_WorldToCamera * in_LocalToWorld;
    ex_Position_v = localToCamera * vec4(ve_Position_l, 1.f);
    gl_Position = u_Projection * ex_Position_v;
    ex_Normal_v = localToCamera * vec4(ve_Normal_l, 0.f);

    ex_Position_lightClip = u_LightViewingMatrix * in_LocalToWorld * vec4(ve_Position_l, 1.f);

    ex_DiffuseColor  = in_Albedo.rgb;
    ex_SpecularColor = in_Albedo.rgb;
    ex_Opacity       = in_Albedo.a;
}