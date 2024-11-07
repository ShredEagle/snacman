#include "Constants.glsl"

struct LightColors
{
    vec4 diffuse;
    vec4 specular;
};


struct DirectionalLight
{
    vec4 direction;
    LightColors colors;
};


struct PointLight
{
    vec4 position;
    vec2 radius; 
    LightColors colors;
};


// Mapping to cpp LightsData
layout(std140, binding = 4) uniform LightsBlock
{
    // LightsDataUser
    uint ub_DirectionalCount;
    uint ub_PointCount;
    vec4 ub_AmbientColor;
    DirectionalLight ub_DirectionalLights[MAX_LIGHTS];
    PointLight ub_PointLights[MAX_LIGHTS];
    // LightsDataInternal
    uint ub_DirectionalLightShadowMapIndices[MAX_LIGHTS];
};


// TODO: could we somehow merge it with LightColors?
struct LightContributions
{
 vec3 diffuse;
 vec3 specular;
};


void scale(inout LightContributions aLightContribution, float aFactor)
{
    aLightContribution.diffuse  = aLightContribution.diffuse  * aFactor;
    aLightContribution.specular = aLightContribution.specular * aFactor;
}


LightContributions applyLight(vec3 aView, vec3 aLightDir, vec3 aShadingNormal,
                              LightColors aColors, float aSpecularExponent)
{
    LightContributions result;

    vec3 h = normalize(aView + aLightDir);

    float nDotL = dot(aShadingNormal, aLightDir);
    result.diffuse = max(0., nDotL) 
                     * aColors.diffuse.rgb;
    // Eliminate specular light bleeding, with a boolean multiplicative factor
    // see: https://computergraphics.stackexchange.com/q/14072/11110
    // Note: this has a major drawback: it introduces an abrupt cut-off between polygons
    // where the sign of nDotL changes, which is very disturbing for smooth surfaces.
    // TODO: find a better solution (maybe a windowing function on the cut-off factor)
    //float isFacingLight = float(nDotL > 0.);
    // Use a smoothstep windowing function instead of boolean value
    float isFacingLight = smoothstep(0.0, 0.1, nDotL);
    result.specular = isFacingLight 
                      * pow(dotPlus(aShadingNormal, h), aSpecularExponent)
                      * aColors.specular.rgb;

    return result;
}


// Compute inverse-square light attenuation multiplied by a windowing function
// see: rtr 4th p113 (5.14) 
float attenuatePoint(PointLight aLight, float aRadius)
{
    return 
        pow(aLight.radius[0] / max(aRadius, aLight.radius[0]),
            2)
        *
        pow(max(0, 
                (1 - pow(aRadius / aLight.radius[1], 4))),
            2);
}

