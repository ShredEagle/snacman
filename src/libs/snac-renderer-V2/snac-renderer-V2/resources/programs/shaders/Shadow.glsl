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

// TODO: should we remove bias, since we rely on polygon offset?
float getShadowAttenuation_sillyFiltering(vec3 fragPosition_lightTex, uint shadowMapIdx, float bias)
{
    const uint offsetsCount = 4;
    const float base = 1.5;
    vec2 offsets[offsetsCount] = vec2[offsetsCount](
        vec2( base / 1024.,  base / 1024.),
        vec2(-base / 1024.,  base / 1024.),
        vec2( base / 1024., -base / 1024.),
        vec2(-base / 1024., -base / 1024.)
    );

    float main = texture(u_ShadowMap, 
                         vec4(fragPosition_lightTex.xy, // uv
                              shadowMapIdx, // array layer
                              fragPosition_lightTex.z - bias /* reference value */));

    //return main;

    float sec = 0;
    for (int i = 0; i != offsetsCount; ++i)
    {
        sec += texture(u_ShadowMap, 
                       vec4(fragPosition_lightTex.xy + offsets[i], // uv
                            shadowMapIdx, // array layer
                            fragPosition_lightTex.z - bias /* reference value */));
    }
    return (2 * main + (sec / 2)) / 4.;
}


float SampleShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, uint shadowMapIdx, float lightDepth, float bias)
{
    vec3 uvz = vec3(
        base_uv + vec2(u, v) * shadowMapSizeInv,
        lightDepth);

    return getShadowAttenuation(uvz, shadowMapIdx, bias);
}


// 5x5 Gaussian filtering (separable filter leveraging the bilinear interpolation hardware)
// see: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
// see: https://github.com/TheRealMJP/Shadows/blob/8bcc4a4bbe232d5f17eda5907b5a7b5425c54430/Shadows/Mesh.hlsl#L524
float getShadowAttenuation_optimizedPcf(vec3 fragPosition_lightTex, uint shadowMapIdx, float bias)
{
    // TODO: dynamic better
    vec2 shadowMapSize = vec2(1024, 1024);
    vec2 shadowMapSizeInv = 1 / shadowMapSize;

    const float lightDepth = fragPosition_lightTex.z;

    vec2 uv = fragPosition_lightTex.xy * shadowMapSize; // texel unit
    // TODO: explain why +0.5?
    const float h = 0.5;
    //const float h = 0.;
    vec2 base_uv = vec2(
        floor(uv.x + h),
        floor(uv.y + h)
    );
    vec2 st = (uv + h) - base_uv;
    float s = st.x;
    float t = st.y;

    base_uv -= h;
    base_uv *= shadowMapSizeInv;

    float receiverPlaneDepthBias = bias + 0.001;

    float sum = 0;

    // Filter size: 5
    float uw0 = (4 - 3 * s);
    float uw1 = 7;
    float uw2 = (1 + 3 * s);

    float u0 = (3 - 2 * s) / uw0 - 2;
    float u1 = (3 + s) / uw1;
    float u2 = s / uw2 + 2;

    float vw0 = (4 - 3 * t);
    float vw1 = 7;
    float vw2 = (1 + 3 * t);

    float v0 = (3 - 2 * t) / vw0 - 2;
    float v1 = (3 + t) / vw1;
    float v2 = t / vw2 + 2;

    sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);

    sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);

    sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);
    sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, shadowMapIdx, lightDepth, receiverPlaneDepthBias);

    return sum / 144;
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
            //getShadowAttenuation(ex_Position_lightTex[shadowMapIdx], shadowMapIdx, bias);
            getShadowAttenuation_optimizedPcf(ex_Position_lightTex[shadowMapIdx], shadowMapIdx, bias);
        scale(aLighting, shadowAttenuation);
    }
}


#endif //SHADOW_GLSL_INCLUDE_GUARD