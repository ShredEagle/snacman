#version 420

#include "Gamma.glsl"

uniform sampler2DRect u_GlyphAtlas;

in vec2 ex_AtlasUv_pix;
in vec4 ex_Color;

out vec4 out_Color;

#define SMOOTH_TEXT
#if defined(SMOOTH_TEXT) 
float smoothing = 0.25 / 2 / 2;
#endif // SMOOTH_TEXT


#define OUTLINE_TEXT
#if defined(OUTLINE_TEXT)
const float outlineFade = 0.1; // the fade-in / out size
const float outlineSolid_half = 0.01; // half of the solid(core) outline width
const vec2 OUTLINE_MIN = vec2(0.5 - outlineSolid_half - outlineFade, 0.5 - outlineSolid_half);
const vec2 OUTLINE_MAX = vec2(0.5 + outlineSolid_half, 0.5 + outlineSolid_half + outlineFade);
const vec3 OUTLINE_COLOR = vec3(0.);
#endif //OUTLINE_TEXT

void main(void)
{
    // Sample the SDF from the glyph atlas
    // TODO rename distance
    float atlasOpacity = texture(u_GlyphAtlas, ex_AtlasUv_pix).r;

#if !defined(SMOOTH_TEXT)
    #if !defined(OUTLINE_TEXT)
        // The simplest case: non-smooth, no-outline
        if (atlasOpacity < 0.5)
        {
            discard;
        }
        out_Color = ex_Color;
    #else // OUTLINE_TEXT && !(SMOOTH_TEXT)
        if (atlasOpacity < OUTLINE_MIN[0])
        {
            discard;
        }
        else if(atlasOpacity <= OUTLINE_MAX[1])
        {
            // This fragment in the outline (opacity between min[0] and max[1])
            out_Color = vec4(OUTLINE_COLOR, ex_Color.a);
        }
        else
        {
            out_Color = ex_Color;
        }
    #endif // OUTLINE_TEXT
#else // SMOOTHING
    #if defined(OUTLINE_TEXT)
        // Smoothing, with outline
        if(atlasOpacity <= OUTLINE_MIN[1])
        {
            // The outline fade-in zone: fades between color-buffer and outline-color
            float oFactor = smoothstep(OUTLINE_MIN[0], OUTLINE_MIN[1], atlasOpacity);
            out_Color = vec4(OUTLINE_COLOR, oFactor * ex_Color.a);
        }
        else
        {
            // The outline solide + fad-out zone: fades between outline-color and text-color

            // Note: interestingly, smoothsteps works with edge0 > edge1
            // (the inversion to a more usual edge0 < edge1 is trivial)
            float oFactor = smoothstep(OUTLINE_MAX[1], OUTLINE_MAX[0], atlasOpacity);
            vec3 albedo = mix(ex_Color.rgb, OUTLINE_COLOR, oFactor);
            out_Color = vec4(albedo, ex_Color.a);
        }
    #else // !(OUTLINE_TEXT)
        // Smoothing, without oultine
        albedo = ex_Color.rgb;
        float opacity = smoothstep(0.5 - smoothing, 0.5 + smoothing, atlasOpacity);
        out_Color = vec4(albedo, ex_Color.a * opacity);
    #endif //OUTLINE_TEXT
#endif // SMOOTH_TEXT

    out_Color = correctGamma(out_Color);
}

#if 0
in vec4 ex_Albedo;
in vec2 ex_AtlasCoords;
in float ex_Scale;

out vec4 out_Color;

uniform sampler2DRect u_FontAtlas;

const float preSmoothing = 0.25 / 2.;

void main(void)
{
    // The right smoothing value is 0.25 / (spread * scale)
    float smoothing = preSmoothing / ex_Scale;
    float atlasOpacity = texture(u_FontAtlas, ex_AtlasCoords).r;
    float sdfOpacity = smoothstep(0.5 - smoothing, 0.5 + smoothing, atlasOpacity);

    // Gamma correction
    float gamma = 2.2;
    out_Color = vec4(pow(ex_Albedo.rgb, vec3(1./gamma)), ex_Albedo.a * sdfOpacity);
    /* out_Color = vec4(pow(ex_Albedo.rgb, vec3(1./gamma)), ex_Albedo.a * atlasOpacity); */
}
#endif