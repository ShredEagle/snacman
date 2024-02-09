#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"
 
in vec3 ex_Position_cam;
in vec3 ex_Normal_cam;

#ifdef VERTEX_COLOR
in vec4 ex_Color;
#endif

in vec2[4] ex_Uv;
in flat uint ex_MaterialIdx;

#ifdef TEXTURED
uniform sampler2DArray u_DiffuseTexture;
#endif

struct PhongMaterial
{
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    uint textureIndex;
    uint diffuseUvChannel;
    float specularExponent;
};

layout(std140, binding = 2) uniform MaterialsBlock
{
    PhongMaterial ub_Phong[128];
};

out vec4 out_Color;


void main()
{
    // Material
    PhongMaterial material = ub_Phong[ex_MaterialIdx];

#ifdef VERTEX_COLOR
    vec4 albedo = ex_Color;
#else
    vec4 albedo = vec4(1., 1., 1., 1.);
#endif

#ifdef TEXTURED
    albedo = albedo * texture(u_DiffuseTexture, vec3(ex_Uv[material.diffuseUvChannel], material.textureIndex));
#endif

    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    if(albedo.a < 0.5)
    {
        discard;
    }

    // TODO take the light(s) as UBO input
    vec3 lightAmbientColor = vec3(0.5, 0.5, 0.5);
    vec3 lightColor = vec3(1., 1., 1.);
    vec3 lightDir_cam = normalize(vec3(-0.5, 0., 1.));

    // Phong reflection model
    vec3 view_cam = vec3(0., 0., 1.);
    vec3 h_cam = normalize(view_cam + lightDir_cam);
    vec3 normal_cam = normalize(ex_Normal_cam);

    vec3 ambient = vec3(material.ambientColor);
    vec3 diffuse = dotPlus(normal_cam, lightDir_cam) * vec3(material.diffuseColor);
    vec3 specular = pow(dotPlus(normal_cam, h_cam), material.specularExponent) * vec3(material.specularColor);

    vec3 phongColor = albedo.rgb * (lightColor * (diffuse + specular) + lightAmbientColor * ambient);
    //out_Color = vec4(ex_Uv, 0., 1.);
    //out_Color = vec4(normal_cam, 1.);
    out_Color = correctGamma(vec4(phongColor, albedo.a * material.diffuseColor.a));
}