#version 420

in vec3 ex_Position_c;
in vec3 ex_Normal_c;
in vec4 ex_Tangent_c;
in vec4 ex_ColorFactor;
#ifdef TEXTURES
in vec2[2] ex_TextureCoords;
#else
in vec4 ex_Albedo;
#endif
#ifdef SHADOW
in vec4 ex_Position_lightClip;
#endif

uniform vec3 u_LightPosition_c;
uniform vec3 u_LightColor = vec3(0.8, 0.0, 0.8);
uniform vec3 u_AmbientColor = vec3(0.2, 0.0, 0.2);
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

float getShadowAttenuation(vec4 fragPosition_lightClip)
{
    // From light clip-space to light texture coordinates [0, 1]
    vec3 fragPosition_shadowTexture = 
        (fragPosition_lightClip.xyz/fragPosition_lightClip.w + 1) / 2;
    fragPosition_shadowTexture.z -= u_ShadowBias;

    return texture(u_ShadowMap, fragPosition_shadowTexture).r;
}
#endif

out vec4 out_Color;


void main(void)
{
    // Everything in camera space
    vec3 light_c = normalize(u_LightPosition_c - ex_Position_c);
    vec3 view_c = vec3(0., 0., 1.);
    vec3 h_c = normalize(view_c + light_c);
    
    float specularExponent = 32;

    vec4 color = 
        ex_ColorFactor 
#ifdef TEXTURES
        * texture(u_BaseColorTexture, ex_TextureCoords[u_BaseColorUVIndex]);
#else
        * ex_Albedo;
#endif
    
    //
    // Compute the fragment normal
    //
#ifdef TEXTURES
    // The normal in tangent space
    vec3 normal_tbn = normalize(
        // Mapping from [0.0, 1.0] to [-1.0, 1.0]
        (texture(u_NormalTexture, ex_TextureCoords[0]).xyz * 2.0 - vec3(1.0))
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
        u_AmbientColor;
    vec3 diffuse = 
        u_LightColor * max(0.f, dot(normal_c, light_c));
    vec3 specular = 
        u_LightColor * pow(max(0.f, dot(normal_c, h_c)), specularExponent);
    vec3 phongColor = 
#ifdef SHADOW
        (ambient + (diffuse + specular) * getShadowAttenuation(ex_Position_lightClip)) * color.xyz;
        
#else
        (ambient + diffuse + specular) * color.xyz;
#endif

    //
    // Gamma correction
    //
    float gamma = 2.2;
    // Gamma compression from linear color space to "simple sRGB", as expected by the monitor.
    // (The monitor will do the gamma expansion.)
    out_Color = vec4(pow(phongColor, vec3(1./gamma)), color.w);
}