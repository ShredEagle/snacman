#version 430

#include "Constants.glsl"
#include "Gamma.glsl"
#include "HelpersIbl.glsl"
#include "HelpersPbr.glsl"

#include "Lights.glsl"
#include "GenericMaterial.glsl"
#include "ViewProjectionBlock.glsl"
 
in vec3 ex_Position_cam;
in vec3 ex_Normal_cam;
in vec3 ex_Tangent_cam;
in vec3 ex_Bitangent_cam;

#ifdef VERTEX_COLOR
in vec4 ex_Color;
#endif

in vec2[4] ex_Uv;
in flat uint ex_MaterialIdx;

#ifdef TEXTURED
uniform sampler2DArray u_DiffuseTexture;
uniform sampler2DArray u_NormalTexture;
uniform sampler2DArray u_MetallicRoughnessAoTexture;
#endif

#ifdef SHADOW_MAPPING
uniform sampler2DArrayShadow u_ShadowMap;
in vec3[MAX_SHADOWS] ex_Position_lightTex;
#endif //SHADOW_MAPPING

#if defined(ENVIRONMENT_MAPPING)
uniform samplerCube u_EnvironmentTexture;
uniform samplerCube u_FilteredRadianceEnvironmentTexture;
uniform samplerCube u_FilteredIrradianceEnvironmentTexture;
uniform sampler2D u_IntegratedEnvironmentBrdf;
#endif //ENVIRONMENT_MAPPING

out vec4 out_Color;

//#define GLTF_BRDF
//#define GLTF_GGX
#define GGX_BRDF
//#define BLINNPHONG_BRDF


/// @brief A material tailored for BRDFs formulas.
/// It is intended as a convenient parameter to PBR light computations.
struct PbrMaterial
{
    vec3 diffuseColor;
    vec3 f0;
    vec3 f90;
    float alpha;
};


//float getShadowAttenuation(vec4 fragPosition_lightClip, uint shadowMapIdx, float bias)
float getShadowAttenuation(vec3 fragPosition_lightTex, uint shadowMapIdx, float bias)
{
    return texture(u_ShadowMap, 
                   vec4(fragPosition_lightTex.xy, // uv
                        shadowMapIdx, // array layer
                        fragPosition_lightTex.z - bias /* reference value */));
}


LightContributions applyLight_pbr(vec3 aView, vec3 aLightDir, vec3 aShadingNormal,
                                  PbrMaterial aMaterial, LightColors aColors)
{
    LightContributions result;

    float alpha = aMaterial.alpha;

#ifdef GLTF_BRDF
    vec3 h = normalize(aView + aLightDir);
    float vDotH = dotPlus(aView, h);
    float hDotL = dotPlus(h, aLightDir);
    float nDotL = dotPlus(aShadingNormal, aLightDir);
    float nDotV = dotPlus(aShadingNormal, aView);
    float nDotH = dotPlus(aShadingNormal, h);

    vec3 reflectance = schlickFresnelReflectance(hDotL, aMaterial.f0, aMaterial.f90);

    //result.diffuse = (1.0 - reflectance) * (diffuseColor / M_PI)
    result.diffuse = (1.0 - reflectance) * (aMaterial.diffuseColor)
                    * nDotL
                    * aColors.diffuse.rgb
                    ;

#ifdef GLTF_GGX
    // glTF-viewer implementation squares the roughness in the functions
    float V = V_GGX_gltf(nDotL, nDotV, alpha);
    float D = D_GGX_gltf(nDotH, alpha);
#else
    float alphaSq = alpha * alpha;
    float V = Visibility_GGX_gltf(alphaSq, hDotL, vDotH, nDotL, nDotV);
    float D = Distribution_GGX_gltf(alphaSq, nDotH);
#endif // GLTF_GGX
    result.specular = reflectance * V * D
                    * nDotL
                    * aColors.specular.rgb
                    ;

#else // not GLTF_BRDF
    vec3 h = normalize(aView + aLightDir);

    float hDotL = dotPlus(h, aLightDir);
    float nDotH = dotPlus(aShadingNormal, h);
    float nDotL = dotPlus(aShadingNormal, aLightDir);
    float nDotV = dotPlus(aShadingNormal, aView);

    vec3 F = schlickFresnelReflectance(hDotL, aMaterial.f0, aMaterial.f90);

    // Important: All albedos are given already multiplied by Pi
    // This already satisfy the Pi factor in the reflectance equation.

    result.diffuse  = diffuseBrdf_weightedLambertian(F, aMaterial.diffuseColor)
                      * aColors.diffuse.rgb
                      * nDotL;

#ifdef GGX_BRDF
    result.specular = specularBrdf_GGX(F, nDotH, nDotL, nDotV, alpha)
                      * aColors.specular.rgb
                      * nDotL;
#elif defined(BLINNPHONG_BRDF)
    float nDotL_raw = dot(aShadingNormal, aLightDir);
    float nDotV_raw = dot(aShadingNormal, aView);

    float alpha_b = alpha / 1.7; // the magic denominator was manually tweaked to mostly match
    result.specular = specularBrdf_BlinnPhong(F, nDotH, nDotL_raw, nDotV_raw, alpha_b)
                      * aColors.specular.rgb
                      * nDotL;
#endif // GGX_BRDF / BLINNPHONG_BRDF
#endif // GLTF_BRDF   
    return result;
}


void main()
{
    // Fetch the material
    GenericMaterial material = ub_MaterialParams[ex_MaterialIdx];

#ifdef VERTEX_COLOR
    vec4 albedo = ex_Color;
#else
    vec4 albedo = vec4(1., 1., 1., 1.);
#endif

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

    // TODO factorize that!

// Defines for BC5 Red-Green texture compression, which only store 2 channels
// The third component has to be reconstructed.
#if defined(TEXTURED)
#define BC5_RGTC;
#if defined(BC5_RGTC)
    // Fetch from Red-Green channels, and remap from [0, 1]^2 to [-1, 1]^2.
    vec2 normalXY = 
        texture(u_NormalTexture, 
                vec3(ex_Uv[material.normalUvChannel], material.normalTextureIndex)).xy
        * 2.0 - vec2(1.0);

    // Derives the third component from the two others, assuming the source normal map data was normalized
    vec3 normal_tbn = vec3(normalXY, sqrt(1.0 - dot(normalXY, normalXY)));
#else
    // Fetch from normal map, and remap from [0, 1]^3 to [-1, 1]^3.
    vec3 normal_tbn = 
        texture(u_NormalTexture, 
                vec3(ex_Uv[material.normalUvChannel], material.normalTextureIndex)).xyz
        * 2 - vec3(1);
#endif // BC5_RGTC
#else // not textured
    vec3 normal_tbn = vec3(0, 0, 1);
#endif // TEXURED

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

//#define COMPUTE_BITANGENT
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

    //out_Color = vec4(bumpNormal_cam, 1.0);
    //out_Color = vec4(remapToRgb(normal_cam), 1.0);
    //out_Color = vec4(remapToRgb(tangent_cam), 1.0);
    //out_Color = vec4(remapToRgb(bitangent_cam), 1.0);
    //out_Color = vec4(remapToRgb(bumpNormal_cam), 1.0);
    //out_Color = vec4(remapToRgb(normal_tbn), 1.0);
    //out_Color = albedo;
    //out_Color = correctGamma(albedo);
    //return;

    vec3 shadingNormal_cam = bumpNormal_cam;
    //vec3 shadingNormal_cam = normalize(normal_cam);

    //
    // Metallic-Roughness parameterization 
    //

    // Extract PBR parameters
#if defined(TEXTURED)
    vec3 mrao = 
        texture(u_MetallicRoughnessAoTexture, 
                vec3(ex_Uv[material.mraoUvChannel], material.mraoTextureIndex)).xyz;
#else // not textured
    vec3 mrao = vec3(0, 0.2, 0);
#endif // TEXTURED

#define PBR_CHANNELS_GLTF

#ifdef PBR_CHANNELS_GLTF
    float metallic = mrao.b;
    float roughness = mrao.g;
    float ambientOcclusion = mrao.r;
#else
    float metallic = mrao.r;
    float roughness = mrao.b;
    float ambientOcclusion = mrao.g;
#endif

    //out_Color = vec4(vec3(metallic), 1.);
    //out_Color = vec4(vec3(roughness), 1.);
    //out_Color = vec4(vec3(ambientOcclusion), 1.);
    //return;

    // Handle alpha

    // Disney PBR square the user provided roughness value (r) to obtain alpha.
    // hard to say if the map already contains it squared, which would save instructions
    // (but would change the precision distribution)
    //
    // Also, from "Real Shading in Unreal Engine 4", some other massaging of roughness are possible.
    // For examplen to reduce "hotness" alpha = ((roughness + 1) / 2) ^ 2
    float alpha = alphaFromRoughness(roughness);
    // Note: Too smooth a surface (i.e too low an alpha)
    // makes it that there is not even a specular highlight showing with most models
    // (at least GGX & Blinn-Phong)
    // So, uncomment to fix it (e.g. the display on the glTF water bottle)
    //alpha = max(0.005, alpha);

    // Material blending approaches
    // see rtr 4th p366

// * When defined, the parameters themselves are blended
//   before evaluating the shading model.
//   Although theoretically incorrect (parameters do not have a linear relation to the output)
//   it is faster and the usual approach in real-time.
// * The alternative (not defined) is evaluation the shading model twice 
//   (once as metal, once as dielectric),
//   then blending the results based on metallic.
#define MATERIAL_BLEND_PARAMETERS  // currently required for IBL

    // Populate the PBR material from metallic-roughness parameterization
#if defined(MATERIAL_BLEND_PARAMETERS)
    // This approach is blending the parameters before computing the model.
    PbrMaterial pbrMaterial;
    pbrMaterial.diffuseColor = mix(albedo.rgb, vec3(0.), metallic);
    pbrMaterial.f0 = mix(gF0_dielec, albedo.rgb, metallic);
    pbrMaterial.f90 = vec3(1.0);
    pbrMaterial.alpha = alpha;
#else
    PbrMaterial metalMaterial;
    metalMaterial.diffuseColor = vec3(0.);
    metalMaterial.f0 = albedo.rgb;
    metalMaterial.f90 = vec3(1.);
    metalMaterial.alpha = alpha;

    PbrMaterial dielecMaterial;
    dielecMaterial.diffuseColor = albedo.rgb;
    dielecMaterial.f0 = vec3(0.04);
    dielecMaterial.f90 = vec3(1.);
    dielecMaterial.alpha = alpha;
#endif


    //
    // PBR shading model (light simulation)
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
    
        float shadowAttenuation = 1.0;
#if defined(SHADOW_MAPPING)
        // TODO handle providing the shadow map texture index with the light
        //if(projectShadow(directional.shadowMapIndex))
        if(directionalIdx < MAX_SHADOWS)
        {
            shadowAttenuation = 
                getShadowAttenuation(ex_Position_lightTex[directionalIdx], directionalIdx, 0.005);
        }
#endif // SHADOW_MAPPING

#if defined(MATERIAL_BLEND_PARAMETERS)
        // evaluate the model only once, with interpolated material parameters
        LightContributions lighting =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           pbrMaterial, directional.colors);
#else
        // when blending the shading model results, the model must be evaluated twice
        LightContributions lightingMetal =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           metalMaterial, directional.colors);

        LightContributions lightingDielec =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           dielecMaterial, directional.colors);

        LightContributions lighting;
        lighting.diffuse += mix(lightingDielec.diffuse, lightingMetal.diffuse, metallic);
        lighting.specular += mix(lightingDielec.specular, lightingMetal.specular, metallic);
#endif

        diffuseAccum += lighting.diffuse * shadowAttenuation;
        specularAccum += lighting.specular * shadowAttenuation;
    }

    // Point lights
    for(uint pointIdx = 0; pointIdx != ub_PointCount.x; ++pointIdx)
    {
        PointLight point = ub_PointLights[pointIdx];

        // see rtr 4th p110 (5.10)
        vec3 lightRay_cam = point.position.xyz - ex_Position_cam;
        float radius = sqrt(dot(lightRay_cam, lightRay_cam));
        vec3 lightDir_cam = lightRay_cam / radius;

#if defined(MATERIAL_BLEND_PARAMETERS)
        // evaluate the model only once, with interpolated material parameters
        LightContributions lighting =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           pbrMaterial, point.colors);
#else
        // when blending the shading model results, the model must be evaluated twice
        LightContributions lightingMetal =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           metalMaterial, point.colors);

        LightContributions lightingDielec =
            applyLight_pbr(view_cam, lightDir_cam, shadingNormal_cam,
                           dielecMaterial, point.colors);

        LightContributions lighting;
        lighting.diffuse += mix(lightingDielec.diffuse, lightingMetal.diffuse, metallic);
        lighting.specular += mix(lightingDielec.specular, lightingMetal.specular, metallic);
#endif

        float falloff = attenuatePoint(point, radius);
        diffuseAccum += lighting.diffuse * falloff;
        specularAccum += lighting.specular * falloff;
    }

    vec3 ambient = albedo.rgb * material.ambientColor.rgb * ub_AmbientColor.rgb * ambientOcclusion; 
    vec3 diffuse  = diffuseAccum * vec3(material.diffuseColor.rgb);
    vec3 specular = specularAccum * vec3(material.specularColor.rgb);

    vec3 fragmentColor = ambient + diffuse + specular;

#if defined(ENVIRONMENT_MAPPING)

//#define ENVIRONMENT_MAPPING_MIRROR 
#define APPROXIMATE_SPECULAR_IBL

    // No need to normalize as long as it is only used to sample a cubemap.
    vec3 reflected_world = mat3(cameraToWorld) * reflect(-view_cam, shadingNormal_cam);
    vec3 shadingNormal_world = mat3(cameraToWorld) * shadingNormal_cam;
    
#if defined(ENVIRONMENT_MAPPING_MIRROR)
    vec3 mirror = texture(u_EnvironmentTexture, worldToCubemap(reflected_world)).rgb;
    // Note: since light direction is the reflection of view around shadingNormal
    // view / light dir can be used interchangeably to compute the Fresnel term.
    fragmentColor += schlickFresnelReflectance(shadingNormal_cam, view_cam, pbrMaterial.f0)
                     * mirror;
#else
    float alphaSquared = pow(max(0.00001, pbrMaterial.alpha), 2);

#if defined(APPROXIMATE_SPECULAR_IBL)
#if 0 // The live variant does not use pre-computed values for the split-sum approximation:
      //is not intended for production rendering, but for debugging the cached approach.
    vec3 specularIbl = approximateSpecularIbl_live(pbrMaterial.f0,
                                 reflected_world,
                                 dotPlus(shadingNormal_cam, view_cam),
                                 alphaSquared,
                                 roughness,
                                 u_EnvironmentTexture);
#else
    vec3 specularIbl = approximateSpecularIbl(pbrMaterial.f0,
                                reflected_world,
                                shadingNormal_world,
                                // Note should be reused from previous computation
                                // (but is currently calculated inside a function)
                                dotPlus(shadingNormal_cam, view_cam),
                                roughness,
                                u_FilteredRadianceEnvironmentTexture,
                                u_IntegratedEnvironmentBrdf);
#endif
#else
    vec3 specularIbl = specularIBL(pbrMaterial.f0,
                                   alphaSquared,
                                   normalize(shadingNormal_world),
                                   normalize(mat3(cameraToWorld) * view_cam),
                                   u_EnvironmentTexture);
#endif // APPROXIMATE_SPECULAR_IBL

    //
    // Diffuse IBL
    //

    // --- LEFT FOR DOCUMENTATION, the first two point have been addressed by integrating the diffuse reflectance equation ---
    // Currently, our diffuse IBL contribution is very bare bone
    // In particular, the following aspects are not yet addressed:
    // * The values retrieved from the irradiance map are used directly as diffuse contribution.
    //   Yet, it seem that physically this is the irradiance arriving at a non-occluded point with same normal,
    //   Not necessarily the radiance redirected toward the camera at this fragment.
    // * A diffuse BRDF might be applied. In particular, see mftpbr p67. 
    //   This point might be what addressed the previous one.
    // * rtr 4th mention that the Fresnel effect can be modeled (p426), after mentionning retrieval.
    //   Yet, as far as I understand, Fresnel effect depends upon light direction, so it could only be
    //   applied while filtering each map texel, not after retrieving the value.
    //   * Note that mftpbr does use Fresnel terms in its diffuses brdf (Fr_DisneyDiffuse),
    //     which is in a term separate from the actual image lighting pre-integration.
    vec3 diffuseIbl = texture(u_FilteredIrradianceEnvironmentTexture, 
                              worldToCubemap(shadingNormal_world)).rgb
                      * pbrMaterial.diffuseColor.rgb;

    // Control the contributions IBL to the final color
    // TODO: good candidates for some uniforms attached to the environment.
    const float specularIblFactor = 1.f;
    const float diffuseIblFactor = 1.f; 

    // Catches the nan and make them more obvious
    if(isnan(specularIbl.r))
    {
        fragmentColor = vec3(1., 0., 1.);
    }
    else
    {
        fragmentColor += specularIbl * specularIblFactor * vec3(material.specularColor.rgb);
        fragmentColor += diffuseIbl  * diffuseIblFactor  * vec3(material.diffuseColor.rgb);
    }

#endif // ENVIRONMENT_MAPPING_MIRROR
#endif // ENVIRONMENT_MAPPING

    out_Color = correctGamma(vec4(fragmentColor, albedo.a * material.diffuseColor.a));
}