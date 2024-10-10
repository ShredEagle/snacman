#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// TODO #cascade_hardcode_4 we need more flexibility, to support from 1 to 4 lights, with several cascades.
// (glsl requires an hardcoded number of invocations, which might not be 4)
layout(invocations = 4) in;

#include "LightViewProjectionBlock.glsl"

in vec3 ex_Position_world[]; // Position input, from the vertex shader

void main()
{
    for(uint vertexIdx = 0; vertexIdx !=3; ++vertexIdx)
    {
        gl_Layer = gl_InvocationID;
        gl_Position =  lightViewProjections[gl_InvocationID] * vec4(ex_Position_world[vertexIdx], 1.0);
        EmitVertex();
    }
    EndPrimitive();
}