#version 420

#include "Gamma.glsl"

in vec3 ex_Position_c;
in vec3 ex_Normal_c;
in vec4 ex_Tangent_c;
in vec4 ex_BaseColorFactor;

in flat uint ex_MaterialIdx;

#ifdef TEXTURES
in vec2[2] ex_TextureCoords;
#endif

in vec4 ex_Color;

#ifdef SHADOW
in vec4 ex_Position_lightClip;
#endif

// Traditionnally, a single term accounting for all lights in the scene
uniform vec3 u_AmbientColor = vec3(0.2, 0.0, 0.2);

// Per-light data
uniform vec3 u_LightPosition_c;
uniform vec3 u_LightColor = vec3(0.8, 0.0, 0.8); // could be split between diffuse and specular itensities

#ifdef TEXTURES
uniform uint u_BaseColorUVIndex;
uniform uint u_NormalUVIndex;
uniform float u_NormalMapScale;

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_NormalTexture;
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
    uint textureIndex;
    uint diffuseUvChannel;
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
    vec3 light_c = normalize(u_LightPosition_c - ex_Position_c);
    vec3 h_c = normalize(view_c + light_c);
    
    vec4 albedo = 
        ex_Color
        * ex_BaseColorFactor // Note: should probably go away
#ifdef TEXTURES
        * texture(u_BaseColorTexture, ex_TextureCoords[u_BaseColorUVIndex])
#endif
        ;

    // TODO: enable alpha testing (atm)
    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    //if(albedo.a < 0.5)
    //{
    //    discard;
    //}
    
    //
    // Compute the fragment normal
    //
#ifdef TEXTURES
    // The normal in tangent space
    vec3 normal_tbn = normalize(
        // Mapping from [0.0, 1.0] to [-1.0, 1.0]
        (texture(u_NormalTexture, ex_TextureCoords[u_NormalUVIndex]).xyz * 2.0 - vec3(1.0))
        * vec3(u_NormalMapScale, u_NormalMapScale, 1.0));

    // MikkT see: http://www.mikktspace.com/
    // Yet, if the tangent and normal were not normalized (as seems to be proposed in mikkt)
    // the result would be abherent with sample gltf assets (e.g. avocado) 
    vec3 normalBase_c = normalize(ex_Normal_c);
    vec3 tangent_c = normalize(ex_Tangent_c.xyz);
    // TODO should it also be normalized? Should the base be "orthogonalized"?
    vec3 bitangent_c = cross(normalBase_c, tangent_c) * ex_Tangent_c.w;
    vec3 normal_c = normalize(normal_tbn.x * tangent_c
        + normal_tbn.y * bitangent_c
        + normal_tbn.z * normalBase_c);
#else
    // cannot normalize in vertex shader, as interpolation changes length.
    vec3 normal_c = normalize(ex_Normal_c);
#endif

    //
    // Phong illumination
    //
    vec3 ambient = 
        u_AmbientColor * material.ambientFactor.xyz;
    vec3 diffuse =
        u_LightColor * max(0.f, dot(normal_c, light_c))
        * material.diffuseFactor.xyz;
    vec3 specular = 
        u_LightColor * pow(max(0.f, dot(normal_c, h_c)), material.specularExponent)
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
