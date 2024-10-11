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
        // The index of the currently rendered shadow map goes to gl_Layer 
        // (thus gl_Layer points to the shadow map in the layered target)
        gl_Layer = gl_InvocationID + int(lightViewProjectionOffset);
        gl_Position =  lightViewProjections[gl_Layer] * vec4(ex_Position_world[vertexIdx], 1.0);
        EmitVertex();
    }
    EndPrimitive();
}