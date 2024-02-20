#version 420

layout(location=0) in vec3 ve_Position_l;
layout(location=1) in vec3 ve_Normal_l;
layout(location=2) in vec4 ve_Tangent_l;

#ifdef TEXTURES
in vec2 ve_TextureCoords0;
in vec2 ve_TextureCoords1;
#endif

// We need a default value of [1, 1, 1, 1], not [0, 0, 0, 1]
// Could be addressed via: https://www.khronos.org/opengl/wiki/Vertex_Specification#Non-array_attribute_values
// At the moment, export our models with a white albedo on vertices by default.
// Note: It could either be a vertex color, or an instance color (ve_ or in_).
in vec4 a_Color_normalized;

#ifdef RIGGING
#include "Rigging.glsl"
#endif

in mat4 in_LocalToWorld;
// Will be required to support non-uniform scaling.
//layout(location=10) in mat4 in_LocalToWorldInverseTranspose;
in uint in_MaterialIdx;

// WARNING: for some reason, the GLSL compiler assigns the same implicit binding
// index to both uniform blocks if we do not set it explicitly.
layout(std140, binding = 0) uniform ViewProjectionBlock
{
    mat4 u_WorldToCamera;
    mat4 u_Projection;
};

#ifdef SHADOW
uniform mat4 u_LightViewingMatrix;
#endif

// TODO #RV2 remove I guess? This is inherited from the times of gltf PBR material
// How do we handle per player color variations?
uniform vec4 u_BaseColorFactor = vec4(1., 1., 1., 1.);

// TODO change _c to _cam
out vec3 ex_Position_c;
out vec3 ex_Normal_c;
out vec4 ex_Tangent_c;
out vec4 ex_BaseColorFactor;
out flat uint ex_MaterialIdx;

#ifdef TEXTURES
out vec2[2] ex_TextureCoords;
#endif

out vec4 ex_Color;

#ifdef SHADOW
out vec4 ex_Position_lightClip;
#endif

void main(void)
{
    mat4 localToWorld =
        in_LocalToWorld
#ifdef RIGGING
        * assembleSkinningMatrix()
#endif
    ;

    // TODO maybe u_WorldToCamera and in_LocalToWorld should be pre-multiplied in client code?
    mat4 localToCamera = u_WorldToCamera * localToWorld;
    vec4 position_c = localToCamera * vec4(ve_Position_l, 1.f);
    gl_Position = u_Projection * position_c;
    ex_Position_c = vec3(position_c);
    ex_Normal_c = mat3(localToCamera) * ve_Normal_l;
    // Tangents are transformed by the normal localToCamera matrix (unlike normals)
    // see: https://www.pbr-book.org/3ed-2018/Geometry_and_Transformations/Applying_Transformations
    ex_Tangent_c = vec4(mat3(localToCamera) * ve_Tangent_l.xyz, ve_Tangent_l.w);

    ex_BaseColorFactor  = u_BaseColorFactor /* TODO multiply by vertex color, when enabled */;
#ifdef TEXTURES
    ex_TextureCoords = vec2[](ve_TextureCoords0, ve_TextureCoords1);
#endif
    ex_Color = a_Color_normalized;
    ex_MaterialIdx = in_MaterialIdx;

#ifdef SHADOW
    ex_Position_lightClip = u_LightViewingMatrix * localToWorld * vec4(ve_Position_l, 1.f);
#endif
}
