#version 420

#include "Constants.glsl"

const vec2 pos_data[4] = vec2[] (
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

const vec2 tex_data[4] = vec2[] (
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

// The instance attribute associating the OpenGL instance
// (in the sense of instanced rendering) to its corresponding Entity.
in uint in_EntityIdx; // Note: cannot be part of the include, which might be used in FS.
#define ENTITY_IDX_ATTRIBUTE in_EntityIdx
#include "Entities.glsl"
out flat uint ex_EntityIdx;

out vec2 ex_Uv;
out vec4 ex_ColorFactor;

void main() 
{
    ex_Uv = tex_data[gl_VertexID];
    gl_Position = getModelTransform() * vec4(pos_data[gl_VertexID], 0.0, 1.0);
    ex_ColorFactor = getEntity().colorFactor;
}