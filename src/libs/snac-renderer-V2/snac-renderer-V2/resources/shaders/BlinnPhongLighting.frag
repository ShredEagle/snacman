#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"
#include "Lights.glsl"
#include "GenericMaterial.glsl"
 
in vec3 ex_Position_cam;
in vec3 ex_Normal_cam;
in vec3 ex_Tangent_cam;
in vec3 ex_Bitangent_cam;

#ifdef VERTEX_COLOR
in vec4 ex_Color;
#endif

#if defined(ENTITIES)
in flat uint ex_EntityIdx; // Note: cannot be part of the include, which might be used in FS.
#define ENTITY_IDX_ATTRIBUTE ex_EntityIdx
#include "Entities.glsl"
#endif //ENTITIES

in vec2[4] ex_Uv;
in flat uint ex_MaterialIdx;

#ifdef TEXTURED
uniform sampler2DArray u_DiffuseTexture;
#if !defined(NAIVE_TEXTURES)
uniform sampler2DArray u_NormalTexture;
#endif //NAIVE_TEXTURES
#endif

out vec4 out_Color;

void main()
{
    // Fetch the material
    GenericMaterial material = ub_MaterialParams[ex_MaterialIdx];

#ifdef VERTEX_COLOR
    vec4 albedo = ex_Color;
#else
    vec4 albedo = vec4(1., 1., 1., 1.);
#endif

#if defined(ENTITIES)
    albedo = albedo * getEntity().colorFactor;
#endif //ENTITIES

#ifdef TEXTURED
    albedo = albedo * texture(u_DiffuseTexture, vec3(ex_Uv[material.diffuseUvChannel], material.diffuseTextureIndex));
#endif

    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    if(albedo.a < 0.5)
    {
        discard;
    }


    //
    // Normal mapping
    //
#if defined(TEXTURED) && !defined(NAIVE_TEXTURES)
    // Fetch from normal map, and remap fro [0, 1]^3 to [-1, 1]^3.
    vec3 normal_tbn = 
        texture(u_NormalTexture, 
                vec3(ex_Uv[material.normalUvChannel], material.normalTextureIndex)).xyz
        * 2 - vec3(1);
#else
    vec3 normal_tbn = vec3(0, 0, 1);
#endif // TEXTURED && !NAIVE_TEXTURES

    // MikkT see: http://www.mikktspace.com/

//#define NORMALIZE_TBN
#ifdef NORMALIZE_TBN
    // Despite MikkT guideline, if the tangent and normal were not normalized
    // the result was be abherent with sample gltf assets (e.g. avocado) 
    vec3 normal_cam = normalize(ex_Normal_cam);
    vec3 tangent_cam = normalize(ex_Tangent_cam);
#else
    // Without normalization
    vec3 normal_cam = ex_Normal_cam;
    vec3 tangent_cam = ex_Tangent_cam;
#endif

#ifdef COMPUTE_BITANGENT
    // TODO handle handedness, which should be -1 or 1
    //float handedness
    vec3 bitangent_cam = cross(normal_cam, tangent_cam) * handedness;
#else
    vec3 bitangent_cam = ex_Bitangent_cam;
#endif

#ifdef NORMALIZE_TBN
    bitangent_cam = normalize(bitangent_cam);
#endif

    vec3 bumpNormal_cam = normalize(
        normal_tbn.x * tangent_cam
        + normal_tbn.y * bitangent_cam
        + normal_tbn.z * normal_cam
    );

    //out_Color = vec4(remapToRgb(normal_cam), 1.0);
    //out_Color = vec4(remapToRgb(tangent_cam), 1.0);
    //out_Color = vec4(remapToRgb(bitangent_cam), 1.0);
    //out_Color = vec4(remapToRgb(bumpNormal_cam), 1.0);
    //out_Color = vec4(remapToRgb(normal_tbn), 1.0);
    //out_Color = albedo;
    //return;

    vec3 shadingNormal_cam = bumpNormal_cam;

    //
    // BlinnPhong reflection model
    //

    // We consider the eye is at the origin in cam space
    vec3 view_cam = -normalize(ex_Position_cam.xyz);

    // Accumulators for the lights contributions
    vec3 diffuseAccum = vec3(0.);
    vec3 specularAccum = vec3(0.);

    // Directional lights
    for(uint directionalIdx = 0; directionalIdx != ub_DirectionalCount.x; ++directionalIdx)
    {
        DirectionalLight directional = ub_DirectionalLights[directionalIdx];
        vec3 lightDir_cam = -directional.direction.xyz;
        
        LightContributions lighting = 
            applyLight(view_cam, lightDir_cam, shadingNormal_cam,
                       directional.colors, material.specularExponent);
        diffuseAccum += lighting.diffuse;
        specularAccum += lighting.specular;
    }

    // Point lights
    for(uint pointIdx = 0; pointIdx != ub_PointCount.x; ++pointIdx)
    {
        PointLight point = ub_PointLights[pointIdx];

        // see rtr 4th p110 (5.10)
        vec3 lightRay_cam = point.position.xyz - ex_Position_cam;
        float r = sqrt(dot(lightRay_cam, lightRay_cam));
        vec3 lightDir_cam = lightRay_cam / r;

        LightContributions lighting = 
            applyLight(view_cam, lightDir_cam, shadingNormal_cam,
                       point.colors, material.specularExponent);

        float falloff = attenuatePoint(point, r);
        diffuseAccum += lighting.diffuse * falloff;
        specularAccum += lighting.specular * falloff;
    }

    vec3 ambient = material.ambientColor.rgb * ub_AmbientColor.rgb;
    vec3 diffuse  = diffuseAccum * vec3(material.diffuseColor.rgb);
    vec3 specular = specularAccum * vec3(material.specularColor.rgb);

#if ALBEDO_SPECULAR
    // The specular highlights are multiplied with the albedo:
    // this gives tinted specular reflection (correct for metals, wrong for dielectrics), and dim highlights for dark surfaces.
    vec3 phongColor = albedo.rgb * (diffuse + specular + ambient);
#else
    vec3 phongColor = albedo.rgb * (diffuse + ambient) + specular;
#endif

    out_Color = correctGamma(vec4(phongColor, albedo.a * material.diffuseColor.a));
}