#version 420

in vec3 ve_Position_l;
in vec2 ve_TextureCoords0;

out vec2 ex_TextureCoords;

void main(void)
{
    gl_Position = vec4(ve_Position_l, 1.f);
    ex_TextureCoords = ve_TextureCoords0;
}