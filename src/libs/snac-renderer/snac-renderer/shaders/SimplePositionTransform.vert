#version 420

in vec3 ve_Position_l;
in vec2 ve_TextureCoords0;

in mat4 in_LocalToWorld;

uniform mat4 u_ViewingMatrix;

out vec2 ex_TextureCoords;

void main(void)
{
    gl_Position = 
        u_ViewingMatrix * in_LocalToWorld * vec4(ve_Position_l, 1.f);

    ex_TextureCoords = ve_TextureCoords0;
}