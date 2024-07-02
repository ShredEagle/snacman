#version 420

layout(location = 0) in vec3 ve_Position_l;

#include "ViewProjectionBlock.glsl"

out vec3 ex_CubeTextureCoords;

void main(void)
{
    // TODO can be replace mat3 with using a homogeneous vector (w = 0)?
    // (Afraid it will break projection)
    vec4 pos = 
        projection
        * mat4(mat3(worldToCamera))
        * vec4(ve_Position_l, 1.0)
        ;

    // Optimization: Setting z component to w ensures the fragment will be at maxdepth
    // i.e. it will be early discarded if another fragment was written
    gl_Position = pos.xyww;

    // The interpolated world coordinate of the fragment will be the direction to sample in cubemap
    ex_CubeTextureCoords = ve_Position_l;
}