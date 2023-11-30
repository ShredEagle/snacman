#version 420

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


layout(location = 0) out vec4  out_Accum;
layout(location = 1) out float out_Revealage;


// TODO what is transmit?
// See: https://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html#3D_transparency_pass
void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ) { 
    // Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
    // transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
    // reflection. This model doesn't handled colored transmission, so it averages the color channels. See 
    //    McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
    //    http://graphics.cs.williams.edu/papers/CSSM/
    // for a full explanation and derivation.

    premultipliedReflect.a *= 1.0 - clamp((transmit.r + transmit.g + transmit.b) * (1.0 / 3.0), 0, 1);

    // You may need to adjust the w function if you have a very large or very small view volume; see the paper and
    //   presentation slides at http://jcgt.org/published/0002/02/09/

    // Intermediate terms to be cubed
    float a = min(1.0, premultipliedReflect.a) * 8.0 + 0.01;
    float b = -gl_FragCoord.z * 0.95 + 1.0;

    // If your scene has a lot of content very close to the far plane,
    //   then include this line (one rsqrt instruction):
    //b /= sqrt(1e4 * abs(csZ));
    float w    = clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e2);
    out_Accum     = premultipliedReflect * w;
    out_Revealage = premultipliedReflect.a;
}


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


    vec4 reflect = vec4(phongColor, 0.9); // Let's hardcode transparency atm
    //vec4 reflect = vec4(phongColor, 1); // Let's hardcode transparency atm
    vec3 transmit = vec3(0); // What is that?
    writePixel(reflect, transmit, 0);
}