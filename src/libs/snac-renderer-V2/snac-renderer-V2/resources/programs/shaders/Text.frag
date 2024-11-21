#version 420

#include "Gamma.glsl"

uniform sampler2DRect u_GlyphAtlas;

in vec2 ex_AtlasUv_tex;
in vec4 ex_Color;
in float ex_Scale;

out vec4 out_Color;

// The alpha value that is considered "on the edge"
// 0.5 is the canonical value here, but effects can be achieved by changing.
const float gBorderCutoff = 0.5;

#define SMOOTH_TEXT
#if defined(SMOOTH_TEXT) 
const float smoothingBase = 0.25 / 5;
#endif // SMOOTH_TEXT


#define OUTLINE_TEXT
#if defined(OUTLINE_TEXT)
const float outlineFade = 0.1; // the fade-in / out size
const float outlineSolid_half = 0.01; // half of the solid(core) outline width
const vec2 OUTLINE_MIN = vec2(-outlineSolid_half - outlineFade, -outlineSolid_half);
const vec2 OUTLINE_MAX = vec2(outlineSolid_half,                 outlineSolid_half + outlineFade);
const vec3 OUTLINE_COLOR = vec3(0.);
#endif //OUTLINE_TEXT

void main(void)
{
    // Sample the SDF from the glyph atlas.
    // The signed distance of this fragment from the border (offset by cutoff), in range [-0.5, 0.5] (unit: texels/(2*spread)).
    // see: https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread
    // Note: FreeType render SDF on 8 bits with [0..127] meaning outside, 128 meaning on edge, and [129..255] meaning inside.
    // So after offseting by cutoff, positive values are inside the glyph, and negative values are outside.
    float signedDistance = texture(u_GlyphAtlas, ex_AtlasUv_tex).r - gBorderCutoff;

#if 1
    // Note: this antialising method, which does not require to produce any scaling factor
    // in the vertex shader, is presented by:
    // https://drewcassidy.me/2020/06/26/sdf-antialiasing/
    // (note: uses reversed sign)

    float spreadDoubled = 8 * 2;
    // Default is 8,
    // see: https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread
    // The signed distance is in texel/(2*spread), see above.
    float distance_texAtlas = signedDistance * spreadDoubled;
    
    // The difference in uv coordinates with neighboring fragment.
    // Already in texels because the UV coordinates of a sampler2DRect are in texels.
    vec2 duv_texel = fwidth(ex_AtlasUv_tex);
    // Note: I do not think the length is the exact metric, but we need to move
    // from the 2-dimension differential to a scalar, since the atlas is a 1-D SDF.
    float texelPerFragment = length(duv_texel);

    float distance_fragmentViewport = distance_texAtlas / texelPerFragment;

    // `distance_fragmentViewport` is from the center of the fragment to the edge of the font.
    // A distance of 0 thus means that the edge passes by the fragment center, leading to half-coverage.
    // 0.5 (resp -0.5) distance means the edge is aligned on the fragment border, fragment being inside (resp. outside).
    // see: https://mortoray.com/antialiasing-with-a-signed-distance-field/
    // (note: uses reversed sign)
    out_Color = vec4(ex_Color.rgb,
                     ex_Color.a * clamp(0.5 + distance_fragmentViewport, 0, 1));
#else

#if defined(OUTLINE_TEXT)
    vec2 oMin = OUTLINE_MIN / ex_Scale;
    vec2 oMax = OUTLINE_MAX / ex_Scale;
#endif //OUTLINE_TEXT

#if !defined(SMOOTH_TEXT)
    #if !defined(OUTLINE_TEXT)
        // The simplest case: non-smooth, no-outline
        if (signedDistance < 0.)
        {
            discard;
        }
        out_Color = ex_Color;
    #else // OUTLINE_TEXT && !(SMOOTH_TEXT)
        if (signedDistance < oMin[0])
        {
            discard;
        }
        else if(signedDistance <= oMax[1])
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
        if(signedDistance <= oMin[1])
        {
            // The outline fade-in zone: fades between color-buffer and outline-color
            float oFactor = smoothstep(oMin[0], oMin[1], signedDistance);
            out_Color = vec4(OUTLINE_COLOR, oFactor * ex_Color.a);
        }
        else
        {
            // The outline solide + fad-out zone: fades between outline-color and text-color

            // Note: interestingly, smoothsteps works with edge0 > edge1
            // (the inversion to a more usual edge0 < edge1 is trivial)
            float oFactor = smoothstep(oMax[1], oMax[0], signedDistance);
            vec3 albedo = mix(ex_Color.rgb, OUTLINE_COLOR, oFactor);
            out_Color = vec4(albedo, ex_Color.a);
        }
    #else // !(OUTLINE_TEXT)
        // Smoothing, without oultine
        float smoothing = smoothingBase / ex_Scale;
        float opacity = smoothstep(-smoothing, smoothing, signedDistance);
        out_Color = vec4(ex_Color.rgb, ex_Color.a * opacity);
    #endif //OUTLINE_TEXT
#endif // SMOOTH_TEXT

#endif // IF 1

    out_Color = correctGamma(out_Color);
}