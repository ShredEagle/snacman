#version 420

#include "Gamma.glsl"

in vec3 ex_Position_c;
in vec3 ex_Normal_c;
in vec4 ex_Tangent_c;
in flat vec4 ex_ColorFactor;

in flat uint ex_MaterialIdx;

#ifdef TEXTURES
in vec2[4] ex_Uvs;
#endif

#ifdef SHADOW
in vec4 ex_Position_lightClip;
#endif

// Per-light data
struct Light
{
    vec3 position_c;
    vec3 color; // could be split between diffuse and specular itensities
};


layout(std140, binding = 4) uniform LightingBlock
{
    // Traditionnally, a single term accounting for all lights in the scene
    vec3 ub_AmbientColor;
    Light ub_Light;
};


#ifdef TEXTURES
uniform sampler2DArray u_DiffuseTexture;

uniform sampler2DArray u_NormalTexture;
uniform float u_NormalMapScale = 1.0;
#endif

#ifdef SHADOW
uniform float u_ShadowBias = 0.;

uniform sampler2DShadow u_ShadowMap;

float getShadowAttenuation(vec4 fragPosition_lightClip, float bias)
{
    // From light clip-space to light texture coordinates [0, 1]
    vec3 fragPosition_shadowTexture = 
        (fragPosition_lightClip.xyz/fragPosition_lightClip.w + 1) / 2;
    
    fragPosition_shadowTexture.z -= bias;

    return texture(u_ShadowMap, fragPosition_shadowTexture).r;
}
#endif

struct PhongMaterial
{
    // Notes: here, the 3 colors of the Phong material are really used as reflection factors.
    vec4 ambientFactor;
    vec4 diffuseFactor;
    vec4 specularFactor;
    uint diffuseTextureIndex;
    uint diffuseUvChannel;
    uint normalTextureIndex;
    uint normalUvChannel;
    float specularExponent;
};

layout(std140, binding = 2) uniform MaterialsBlock
{
    PhongMaterial ub_Phong[128];
};

out vec4 out_Color;


void main(void)
{
    // Material
    PhongMaterial material = ub_Phong[ex_MaterialIdx];

    // Everything in camera space
    const vec3 view_c = vec3(0., 0., 1.);
    vec3 light_c = normalize(ub_Light.position_c - ex_Position_c);
    vec3 h_c = normalize(view_c + light_c);
    
    vec4 albedo =
        ex_ColorFactor // (equivalent to multiplying implicit white by this color factor)
#ifdef TEXTURES
        * texture(u_DiffuseTexture, vec3(ex_Uvs[material.diffuseUvChannel], material.diffuseTextureIndex))
#endif
        ;

    // TODO #alphatest enable alpha testing (atm)
    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    //if(albedo.a < 0.5)
    //{
    //    discard;
    //}
    
    //
    // Compute the fragment normal
    //
#ifdef TEXTURES
    vec3 normal_c;
    // For the moment, let's go with branching to enable (or not) tangents
    if(material.normalUvChannel == 0)
    {
        // The normal in tangent space
        vec3 normal_tbn = normalize(
            // Mapping from [0.0, 1.0] to [-1.0, 1.0]
            // TODO get the normal UV channel index
            (texture(u_NormalTexture, vec3(ex_Uvs[material.normalUvChannel], material.normalTextureIndex)).xyz * 2.0 - vec3(1.0))
            * vec3(u_NormalMapScale, u_NormalMapScale, 1.0));

        // MikkT see: http://www.mikktspace.com/
        // Yet, if the tangent and normal were not normalized (as seems to be proposed in mikkt)
        // the result would be abherent with sample gltf assets (e.g. avocado) 
        vec3 normalBase_c = normalize(ex_Normal_c);
        vec3 tangent_c = normalize(ex_Tangent_c.xyz);
        // TODO should it also be normalized? Should the base be "orthogonalized"?
        vec3 bitangent_c = cross(normalBase_c, tangent_c) * ex_Tangent_c.w;
        normal_c = normalize(normal_tbn.x * tangent_c
            + normal_tbn.y * bitangent_c
            + normal_tbn.z * normalBase_c);
    }
    else
    {
        normal_c = normalize(ex_Normal_c);
    }
#else
    // cannot normalize in vertex shader, as interpolation changes length.
    vec3 normal_c = normalize(ex_Normal_c);
#endif

    //
    // Phong illumination
    //
    vec3 ambient = 
        ub_AmbientColor * material.ambientFactor.xyz;
    vec3 diffuse =
        ub_Light.color * max(0.f, dot(normal_c, light_c))
        * material.diffuseFactor.xyz;
    vec3 specular = 
        ub_Light.color * pow(max(0.f, dot(normal_c, h_c)), material.specularExponent)
        * material.specularFactor.xyz;

#ifdef SHADOW
        float bias = max(8 * (1 - dot(normalize(ex_Normal_c), light_c)), 1) * u_ShadowBias;
    vec3 phongColor = 
        (ambient + (diffuse + specular) * getShadowAttenuation(ex_Position_lightClip, bias)) * albedo.rgb;
#else
    vec3 phongColor = 
        (ambient + diffuse + specular) * albedo.rgb;
#endif

    //
    // Gamma correction
    //
    out_Color = correctGamma(vec4(phongColor, albedo.a));
}
