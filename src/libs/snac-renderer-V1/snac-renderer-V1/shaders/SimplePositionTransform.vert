#version 420

in vec3 ve_Position_l;
in vec4 ve_Albedo;
in vec2 ve_TextureCoords0;

#ifdef MODEL_MATRIX
in mat4 in_LocalToWorld;
#endif

#ifdef RIGGING
#include "Rigging.glsl"
#endif

uniform mat4 u_ViewingMatrix;

out vec4 ex_Albedo;
out vec2 ex_TextureCoords;

void main(void)
{
    gl_Position = 
        u_ViewingMatrix  
#ifdef MODEL_MATRIX
        * in_LocalToWorld
#endif
#ifdef RIGGING
        * assembleSkinningMatrix()
#endif
        * vec4(ve_Position_l, 1.f)
        ;

    ex_Albedo = ve_Albedo;
    ex_TextureCoords = ve_TextureCoords0;
}