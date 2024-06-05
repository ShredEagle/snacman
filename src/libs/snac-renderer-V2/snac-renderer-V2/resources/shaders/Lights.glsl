struct DirectionalLight
{
    vec4 direction;
    vec4 diffuseColor;
    vec4 specularColor;
};


struct PointLight
{
    vec4 position;
    vec2 radius; 
    vec4 diffuseColor;
    vec4 specularColor;
};


#define MAX_LIGHTS 16
layout(std140, binding = 4) uniform LightsBlock
{
    uint ub_DirectionalCount;
    uint ub_PointCount;
    vec4 ub_AmbientColor;
    DirectionalLight ub_DirectionalLights[MAX_LIGHTS];
    PointLight ub_PointLights[MAX_LIGHTS];
};


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

