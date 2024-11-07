#if !defined(SHADOW_GLSL_INCLUDE_GUARD)
#define SHADOW_GLSL_INCLUDE_GUARD


// Should already be included, we save so many cycles not checking the include guard (:
//#include "Lights.glsl"

uniform sampler2DArrayShadow u_ShadowMap;
in vec3[MAX_SHADOW_MAPS] ex_Position_lightTex;


// TODO: should we remove bias, since we rely on polygon offset?
float getShadowAttenuation(vec3 fragPosition_lightTex, uint shadowMapIdx, float bias)
{
    return texture(u_ShadowMap, 
                   vec4(fragPosition_lightTex.xy, // uv
                        shadowMapIdx, // array layer
                        fragPosition_lightTex.z - bias /* reference value */));
}


// High-level function: scale the light contribution by the shadow attenuation factor
void applyShadowToLighting(
    inout LightContributions aLighting,
    uint directionalIdx
#if defined(SHADOW_CASCADE)
    , uint aShadowCascadeIdx
#endif //SHADOW_CASCADE
)
{
    // TODO handle providing the shadow map texture index with the light
    uint shadowMapBaseIdx = ub_DirectionalLightShadowMapIndices[directionalIdx];
    if(shadowMapBaseIdx != INVALID_INDEX)
    {
#if defined(SHADOW_CASCADE)
        // CASCADES_PER_SHADOW multiplication is already done in shadowMapBaseIdx
        uint shadowMapIdx = shadowMapBaseIdx + aShadowCascadeIdx;
#else // SHADOW_CASCADE
        uint shadowMapIdx = shadowMapBaseIdx;
#endif //SHADOW_CASCADE
        const float bias = 0.; // we rely on slope-scale bias, see glPolygonOffset
        float shadowAttenuation = 
            getShadowAttenuation(ex_Position_lightTex[shadowMapIdx], shadowMapIdx, bias);
        scale(aLighting, shadowAttenuation);
    }
}


#endif //SHADOW_GLSL_INCLUDE_GUARD