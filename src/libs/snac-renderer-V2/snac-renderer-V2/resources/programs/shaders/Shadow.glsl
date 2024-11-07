#if !defined(SHADOW_GLSL_INCLUDE_GUARD)
#define SHADOW_GLSL_INCLUDE_GUARD


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


#endif //SHADOW_GLSL_INCLUDE_GUARD