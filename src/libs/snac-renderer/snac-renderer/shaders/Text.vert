#version 420

layout(location=0) in vec3 ve_Position_l;
layout(location=1) in vec2 ve_TextureCoords_t;

layout(location= 4) in mat4  in_LocalToWorld;
layout(location= 8) in vec4  in_Albedo;
layout(location= 9) in ivec2 in_TextureOffset;
layout(location=10) in vec2  in_BoundingBox;

out vec2 ex_AtlasCoords;
out vec4 ex_Albedo;

void main(void)
{
    gl_Position  = in_LocalToWorld * vec4(ve_Position_l, 1.f);
    ex_AtlasCoords = in_TextureOffset + (ve_TextureCoords_t * in_BoundingBox);
    ex_Albedo = in_Albedo;
}