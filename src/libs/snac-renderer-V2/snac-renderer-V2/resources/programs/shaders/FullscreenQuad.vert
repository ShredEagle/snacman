#version 420

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

out vec2 ex_Uv;

void main() 
{
    ex_Uv = tex_data[gl_VertexID];
    gl_Position = vec4(pos_data[gl_VertexID], 0.0, 1.0);
}