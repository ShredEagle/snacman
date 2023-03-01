#version 420

layout(location=0) in vec3 ve_Position_l;
layout(location=1) in vec3 ve_Normal_l;
layout(location=2) in vec4 ve_Tangent_l;
layout(location=3) in vec2 ve_TextureCoords0;
layout(location=4) in vec2 ve_TextureCoords1;
// Not enabled for for the moment (could be taken from COLOR_0 I suppose)
// because we need a default value of [1, 1, 1, 1]
// Could be addressed via: https://www.khronos.org/opengl/wiki/Vertex_Specification#Non-array_attribute_values
//layout(location=3) in vec4 in_Albedo;

layout(location=5) in mat4 in_LocalToWorld;
// Will be required to support non-uniform scaling.
//layout(location=9) in mat4 in_LocalToWorldInverseTranspose;

layout(std140) uniform ViewingBlock
{
    mat4 u_WorldToCamera;
    mat4 u_Projection;
};

uniform vec4 u_BaseColorFactor;

out vec3 ex_Position_c;
out vec3 ex_Normal_c;
out vec4 ex_Tangent_c;
out vec2[2] ex_TextureCoords;
out vec4 ex_ColorFactor;

void main(void)
{
    // TODO maybe should be pre-multiplied in client code?
    mat4 localToCamera = u_WorldToCamera * in_LocalToWorld;
    vec4 position_c = localToCamera * vec4(ve_Position_l, 1.f);
    gl_Position = u_Projection * position_c;
    ex_Position_c = vec3(position_c);
    ex_Normal_c = mat3(localToCamera) * ve_Normal_l;
    // Tangents are transformed by the normal localToCamera matrix (unlike normals)
    // see: https://www.pbr-book.org/3ed-2018/Geometry_and_Transformations/Applying_Transformations
    ex_Tangent_c = vec4(mat3(localToCamera) * ve_Tangent_l.xyz, ve_Tangent_l.w);

    ex_ColorFactor  = u_BaseColorFactor /* TODO multiply by vertex color, when enabled */;
    ex_TextureCoords = vec2[](ve_TextureCoords0, ve_TextureCoords1);
}