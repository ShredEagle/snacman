#version 420

#include "Cube.glsl"
#include "ViewProjectionBlock.glsl"

out vec3 ex_FragmentPosition_world;

void main(void)
{
    //vec3 position_local = gCubePositions[gCubeIndices[gl_VertexID]];
    vec3 position_local = gCubePositions[gCubeIndices_triangleStrip[gl_VertexID]];

    // TODO can be replace mat3 with using a homogeneous vector (w = 0)?
    // (Afraid it will break projection)
    vec4 pos = 
        projection
        * mat4(mat3(worldToCamera))
        * vec4(position_local, 1.0)
        ;

    // Optimization: Setting z component to w ensures the fragment will be at maxdepth
    // i.e. it will be early discarded if another fragment was written
    gl_Position = pos.xyww;

    // The interpolated world coordinate of the fragment will be the direction to sample in cubemap
    ex_FragmentPosition_world = position_local;
}