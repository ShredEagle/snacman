#version 420

in vec4 ex_Position_v;
in vec4 ex_Normal_v;
in vec4 ex_Position_lightClip;
in vec3 ex_DiffuseColor;
in vec3  ex_SpecularColor;
in float ex_Opacity;

uniform float u_ShadowBias = 0.;

uniform vec3 u_LightPosition_v;
uniform vec3 u_LightColor = vec3(0.8, 0.0, 0.8);
uniform vec3 u_AmbientColor = vec3(0.2, 0.0, 0.2);

// TODO use yet another dedicated semantic?
uniform sampler2DShadow u_ShadowMap;

out vec4 out_Color;

float getShadowAttenuation(vec4 fragPosition_lightClip)
{
    // From light clip-space to light texture coordinates [0, 1]
    vec3 fragPosition_shadowTexture = 
        (fragPosition_lightClip.xyz/fragPosition_lightClip.w + 1) / 2;
    fragPosition_shadowTexture.z -= u_ShadowBias;

    return texture(u_ShadowMap, fragPosition_shadowTexture).r;
}

void main(void)
{
    // Everything in camera space
    vec3 light = normalize(u_LightPosition_v - ex_Position_v.xyz);
    vec3 view = vec3(0., 0., 1.);
    vec3 h = normalize(view + light);
    vec3 normal = normalize(ex_Normal_v.xyz); // cannot normalize in vertex shader, as interpolation changes length.
    
    float specularExponent = 32;

    vec3 diffuse = 
        ex_DiffuseColor * (u_AmbientColor + u_LightColor * max(0.f, dot(normal, light)));
    vec3 specular = 
        u_LightColor * ex_SpecularColor * pow(max(0.f, dot(normal, h)), specularExponent);
    vec3 color = (diffuse + specular) * getShadowAttenuation(ex_Position_lightClip);

    // Gamma correction
    float gamma = 2.2;
    out_Color = vec4(pow(color, vec3(1./gamma)), ex_Opacity);
}