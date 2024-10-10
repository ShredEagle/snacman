#version 420

#include "Constants.glsl"

in vec3 ve_Position_local;
in vec3 ve_Normal_local;
in vec3 ve_Tangent_local;
in vec3 ve_Bitangent_local;
in vec4 ve_Color;
in vec2 ve_Uv;

in uint in_ModelTransformIdx;
in uint in_MaterialIdx;

#include "ViewProjectionBlock.glsl"

// TODO #ssbo Use a shader storage block, due to the unbounded nature of the number of instances
layout(std140, binding = 1) uniform LocalToWorldBlock
{
    mat4 modelTransforms[512];
};


#ifdef SHADOW_MAPPING

#include "LightViewProjectionBlock.glsl"

out vec3[MAX_SHADOW_MAPS] ex_Position_lightTex;

#endif //SHADOW_MAPPING

#if defined(TRANSFORM_TO_WORLD)
out vec3 ex_Position_world;
#else
out vec3 ex_Position_cam;
out vec3 ex_Normal_cam;
out vec3 ex_Tangent_cam;
out vec3 ex_Bitangent_cam;
#endif //TRANSFORM_TO_WORLD

out vec4 ex_Color;
out vec2[4] ex_Uv; // simulates 4 UV channels, not implemented atm
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

#if defined(SHADOW_MAPPING) || defined(TRANSFORM_TO_WORLD)
    vec4 position_world = localToWorld * vec4(ve_Position_local, 1.f); // TODO can be consolidated with the main code
#endif //SHADOW_MAPPING || TRANSFORM_TO_WORLD

#if defined(TRANSFORM_TO_WORLD)
    ex_Position_world = position_world.xyz;
#else
    mat4 localToCamera = worldToCamera * localToWorld;
    vec4 position_cam = localToCamera * vec4(ve_Position_local, 1.f);
    gl_Position = projection * position_cam;
    ex_Position_cam = position_cam.xyz;
    // TODO Handle non-orthogonal transformations (requiring transpose of the adjoint, see rtr 4th 4.1.7 p68)
    ex_Normal_cam = normalize(mat3(localToCamera) * ve_Normal_local);
    // Tangents are transformed like positions, by the localToCamera matrix (unlike normals)
    // see: https://www.pbr-book.org/3ed-2018/Geometry_and_Transformations/Applying_Transformations
    ex_Tangent_cam = normalize(mat3(localToCamera) * ve_Tangent_local);
    ex_Bitangent_cam = normalize(mat3(localToCamera) * ve_Bitangent_local);
#endif //TRANSORM_TO_WORLD

    ex_Color = ve_Color;
    ex_Uv[0] = ve_Uv; // only 1 uv channel input at the moment
    ex_MaterialIdx = in_MaterialIdx;

#ifdef SHADOW_MAPPING
    for(uint lightIdx = 0; lightIdx != lightViewProjectionCount; ++lightIdx)
    {
        vec4 position_lightClip = lightViewProjections[lightIdx] * position_world;
        // The position is in homogeneous clip space (from the light POV).
        // We apply the perspective divide to get to NDC, and remap from [-1, 1]^3 to [0, 1]^3.
        // Note: even the depth (z) is remapped to [0, 1], because the viewport transformation
        // did it for the depth values stored in the shadow map.
        ex_Position_lightTex[lightIdx] = 
            (position_lightClip.xyz / position_lightClip.w + 1.) / 2.;
    }
#endif //SHADOW_MAPPING

}
