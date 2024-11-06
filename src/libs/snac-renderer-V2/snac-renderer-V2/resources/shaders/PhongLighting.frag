#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"
#include "GenericMaterial.glsl"
 
in vec3 ex_Position_cam;
in vec3 ex_Normal_cam;

#ifdef VERTEX_COLOR
in vec4 ex_Color;
#endif

#ifdef TEXTURED
in vec2[4] ex_Uv;
#endif // TEXTURED

in flat uint ex_MaterialIdx;

#ifdef TEXTURED
uniform sampler2DArray u_DiffuseTexture;
#endif

out vec4 out_Color;


void main()
{
    // Fetch the material
    GenericMaterial material = ub_MaterialParams[ex_MaterialIdx];

#ifdef VERTEX_COLOR
    vec4 albedo = ex_Color;
#else
    vec4 albedo = vec4(1., 1., 1., 1.);
#endif

#ifdef TEXTURED
    albedo = albedo * texture(u_DiffuseTexture, vec3(ex_Uv[material.diffuseUvChannel], material.diffuseTextureIndex));
#endif // TEXTURED

    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    if(albedo.a < 0.5)
    {
        discard;
    }

    // TODO take the light(s) as UBO input
    vec3 lightAmbientColor = vec3(0.5, 0.5, 0.5);
    vec3 lightColor = vec3(1., 1., 1.);
    vec3 lightDir_cam = normalize(vec3(0., 0.25, 1.));

    // Phong reflection model
#ifdef WRONG_VIEW
    vec3 view_cam = vec3(0., 0., 1.);
#else
    vec3 view_cam = -normalize(ex_Position_cam).xyz;
#endif
    vec3 normal_cam = normalize(ex_Normal_cam);

//#define BLINN
#ifdef BLINN
    vec3 h_cam = normalize(view_cam + lightDir_cam);
#else
    vec3 r_cam = reflect(-lightDir_cam, normal_cam);
#endif

    vec3 ambient = vec3(material.ambientColor);
    vec3 diffuse = dotPlus(normal_cam, lightDir_cam) * vec3(material.diffuseColor);
    vec3 specular = 
#ifdef BLINN
    pow(dotPlus(normal_cam, h_cam), material.specularExponent)
#else // Plain Phong
    // Note: specular exponent is divided by 4, to give results comparables to Blinn
    pow(dotPlus(view_cam, r_cam), material.specularExponent/4) 
#endif
    * vec3(material.specularColor);

    vec3 phongColor = albedo.rgb * (lightColor * (diffuse + specular) + lightAmbientColor * ambient);
    out_Color = correctGamma(vec4(phongColor, albedo.a * material.diffuseColor.a));
}