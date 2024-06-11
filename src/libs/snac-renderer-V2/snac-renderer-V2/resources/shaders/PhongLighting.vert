#version 420

in vec3 ve_Position_local;
in vec3 ve_Normal_local;
in vec3 ve_Tangent_local;
in vec3 ve_Bitangent_local;
in vec4 ve_Color;
in vec2 ve_Uv;

in uint in_ModelTransformIdx;
in uint in_MaterialIdx;

// WARNING: for some reason, the GLSL compiler assigns the same implicit binding
// index to both uniform blocks if we do not set it explicitly.
layout(std140, binding = 0) uniform ViewProjectionBlock
{
    mat4 worldToCamera;
    mat4 projection;
    mat4 viewingProjection;
};

// TODO #ssbo Use a shader storage block, due to the unbounded nature of the number of instances
layout(std140, binding = 1) uniform LocalToWorldBlock
{
    mat4 modelTransforms[512];
};

out vec3 ex_Position_cam;
out vec3 ex_Normal_cam;
out vec3 ex_Tangent_cam;
out vec3 ex_Bitangent_cam;
out vec4 ex_Color;
out vec2[4] ex_Uv; // simulates 4 UV channels, not implement atm
out flat uint ex_MaterialIdx;


#ifdef RIGGING
#include "Rigging.glsl"
#endif


void main(void)
{
    mat4 localToWorld = 
        modelTransforms[in_ModelTransformIdx]
#ifdef RIGGING
        * assembleSkinningMatrix()
#endif
    ;

    mat4 localToCamera = worldToCamera * localToWorld;
    vec4 position_cam = localToCamera * vec4(ve_Position_local, 1.f);
    gl_Position = projection * position_cam;
    ex_Position_cam = vec3(position_cam);
    // TODO Handle non-orthogonal transformations (requiring transpose of the adjoint, see rtr 4th 4.1.7 p68)
    ex_Normal_cam = mat3(localToCamera) * ve_Normal_local;
    // Tangents are transformed like positions, by the localToCamera matrix (unlike normals)
    // see: https://www.pbr-book.org/3ed-2018/Geometry_and_Transformations/Applying_Transformations
    ex_Tangent_cam = mat3(localToCamera) * ve_Tangent_local;
    ex_Bitangent_cam = mat3(localToCamera) * ve_Bitangent_local;
    ex_Color = ve_Color;
    ex_Uv[0] = ve_Uv; // only 1 uv channel input at the moment
    ex_MaterialIdx = in_MaterialIdx;
}
