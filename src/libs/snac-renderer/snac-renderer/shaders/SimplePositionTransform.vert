#version 420

in vec3 ve_Position_l;
in vec2 ve_TextureCoords0;

in mat4 in_LocalToWorld;

// NOTE: It would be better to directly get the assembled matrix.
layout(std140) uniform ViewingBlock
{
    mat4 u_WorldToCamera;
    mat4 u_Projection;
};

out vec2 ex_TextureCoords;

void main(void)
{
    gl_Position = 
        u_Projection * u_WorldToCamera * in_LocalToWorld * vec4(ve_Position_l, 1.f);

    ex_TextureCoords = ve_TextureCoords0;
}