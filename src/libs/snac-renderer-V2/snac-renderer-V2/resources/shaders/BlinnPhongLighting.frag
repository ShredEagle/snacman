#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"
#include "Lights.glsl"
#include "PhongMaterial.glsl"
 
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

out vec4 out_Color;

void main()
{
    // Fetch the material
    PhongMaterial material = ub_Phong[ex_MaterialIdx];

#ifdef VERTEX_COLOR
    vec4 albedo = ex_Color;
#else
    vec4 albedo = vec4(1., 1., 1., 1.);
#endif

#ifdef TEXTURED
    albedo = albedo * texture(u_DiffuseTexture, vec3(ex_Uv[material.diffuseUvChannel], material.diffuseTextureIndex));
#endif

    // Implement "cut-out" transparency: everything below 50% opacity is discarded (i.e. no depth write).
    if(albedo.a < 0.5)
    {
        discard;
    }


    //
    // BlinnPhong reflection model
    //

    // We consider the eye is at the origin in cam space
    vec3 view_cam = -normalize(ex_Position_cam.xyz);

    vec3 normal_cam = normalize(ex_Normal_cam);

    // Accumulators
    vec3 diffuseAccum = vec3(0.);
    vec3 specularAccum = vec3(0.);

    // Directional lights
    for(uint directionalIdx = 0; directionalIdx != ub_DirectionalCount.x; ++directionalIdx)
    {
        DirectionalLight directional = ub_DirectionalLights[directionalIdx];
        vec3 lightDir_cam = -directional.direction.xyz;
        vec3 h_cam = normalize(view_cam + lightDir_cam);

        diffuseAccum  += dotPlus(normal_cam, lightDir_cam) 
                         * directional.diffuseColor.rgb;
        specularAccum += pow(dotPlus(normal_cam, h_cam), material.specularExponent) 
                         * directional.specularColor.rgb;
    }

    // Point lights
    for(uint pointIdx = 0; pointIdx != ub_PointCount.x; ++pointIdx)
    {
        PointLight point = ub_PointLights[pointIdx];

        // see rtr 4th p110 (5.10)
        vec3 lightRay_cam = point.position.xyz - ex_Position_cam;
        float r = sqrt(dot(lightRay_cam, lightRay_cam));
        vec3 lightDir_cam = lightRay_cam / r;
        vec3 h_cam = normalize(view_cam + lightDir_cam);

        float falloff = attenuatePoint(point, r);

        diffuseAccum  += dotPlus(normal_cam, lightDir_cam) 
                         * point.diffuseColor.rgb
                         * falloff;
        specularAccum += pow(dotPlus(normal_cam, h_cam), material.specularExponent) 
                         * point.specularColor.rgb
                         * falloff;
    }

    vec3 ambient = material.ambientColor.rgb * ub_AmbientColor.rgb;
    vec3 diffuse  = diffuseAccum * vec3(material.diffuseColor.rgb);
    vec3 specular = specularAccum * vec3(material.specularColor.rgb);

#if ALBEDO_SPECULAR
    // The specular highlights are multiplied with the albedo:
    // this gives tinted specular reflection (correct for metals, wrong for dielectrics), and dim highlights for dark surfaces.
    vec3 phongColor = albedo.rgb * (diffuse + specular + ambient);
#else
    vec3 phongColor = albedo.rgb * (diffuse + ambient) + specular;
#endif

    out_Color = correctGamma(vec4(phongColor, albedo.a * material.diffuseColor.a));
}