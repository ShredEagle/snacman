#version 420

in vec3 ve_Position_l;
in vec4 a_Color;
in vec2 ve_TextureCoords0;

#ifdef MODEL_MATRIX
in mat4 in_ModelTransform;
#endif

#ifdef RIGGING
#include "Rigging.glsl"
#endif

#include "ViewProjectionBlock.glsl"

out vec4 ex_Color;
out vec2 ex_TextureCoords;

void main(void)
{
    gl_Position = 
        viewingProjection  
#ifdef MODEL_MATRIX
        * in_ModelTransform
#endif
#ifdef RIGGING
        * assembleSkinningMatrix()
#endif
        * vec4(ve_Position_l, 1.f)
        ;

    ex_Color = a_Color;
    ex_TextureCoords = ve_TextureCoords0;
}