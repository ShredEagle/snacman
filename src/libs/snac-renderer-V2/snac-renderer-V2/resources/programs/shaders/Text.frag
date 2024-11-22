#version 420

#include "Constants.glsl"
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

#define OUTLINE_TEXT
#if defined(OUTLINE_TEXT)
// half of the solid(core) outline width, in fragment unit
const float halfOutline_fragment = 1.2; 
const vec3 OUTLINE_COLOR = vec3(0.);
#endif //OUTLINE_TEXT


/// @param signedDistance in fragment unit, with positive being inside.
float computeDistanceInAtlasSpread(float signedDistance_fragment)
{
    // The difference in uv coordinates with neighboring fragment.
    // Already in texels because the UV coordinates of a sampler2DRect are in texels.
    vec2 duv_texel = fwidth(ex_AtlasUv_tex);
    // Note: I do not think the length is the exact metric, but we need to move
    // from the 2-dimension differential to a scalar, since the atlas is a 1-D SDF.
    float texelPerFragment = length(duv_texel);

    float distance_texel = signedDistance_fragment * texelPerFragment;

    // The signed distance is in texel/(2*spread).
    float distance_AtlasSpread = distance_texel / SDF_DOUBLE_SPREAD;
    
    return distance_AtlasSpread;
}


/// @param signedDistance value in atlas spread on [-0.5..0.5], with positive being inside.
float computeDistanceInFragmentUnit(float signedDistance)
{
    // The signed distance is in texel/(2*spread).
    float distance_texAtlas = signedDistance * SDF_DOUBLE_SPREAD;
    
    // The difference in uv coordinates with neighboring fragment.
    // Already in texels because the UV coordinates of a sampler2DRect are in texels.
    vec2 duv_texel = fwidth(ex_AtlasUv_tex);
    // Note: I do not think the length is the exact metric, but we need to move
    // from the 2-dimension differential to a scalar, since the atlas is a 1-D SDF.
    float texelPerFragment = length(duv_texel);

    float distance_fragmentViewport = distance_texAtlas / texelPerFragment;
    return distance_fragmentViewport;
}


/// @param signedDistance value on [-0.5..0.5], with positive being inside.
float computeFragmentCoverage(float signedDistance)
{
    // Note: this antialising method does not require to produce any scaling factor
    // in the vertex shader. It is presented by:
    // https://drewcassidy.me/2020/06/26/sdf-antialiasing/
    // (note: uses reversed sign)

    float distance_fragmentViewport = computeDistanceInFragmentUnit(signedDistance);

    // `distance_fragmentViewport` is from the center of the fragment to the edge of the font.
    // A distance of 0 thus means that the edge passes by the fragment center, leading to half-coverage.
    // 0.5 (resp -0.5) distance means the edge is aligned on the fragment border, fragment being inside (resp. outside).
    // see: https://mortoray.com/antialiasing-with-a-signed-distance-field/
    // (note: uses reversed sign)
    return clamp(0.5 + distance_fragmentViewport, 0, 1);
}


void main(void)
{
    // Sample the SDF from the glyph atlas.
    // The signed distance of this fragment from the border (offset by cutoff), in range [-0.5, 0.5] (unit: texels/(2*spread)).
    // see: https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread
    // Note: FreeType render SDF on 8 bits with [0..127] meaning outside, 128 meaning on edge, and [129..255] meaning inside.
    // So after offseting by cutoff, positive values are inside the glyph, and negative values are outside.
    float signedDistance = texture(u_GlyphAtlas, ex_AtlasUv_tex).r - gBorderCutoff;

#if !defined(SMOOTH_TEXT)
    #if !defined(OUTLINE_TEXT)
        // The simplest case: non-smooth, no-outline
        if (signedDistance < 0.)
        {
            discard;
        }
        out_Color = ex_Color;
    #else // OUTLINE_TEXT && !(SMOOTH_TEXT)
        // Note: It is primordial to clamp the outline half-width so it cannot reach
        // SDF-min nor SDF-max. Otherwise, all values of the atlas would be detected as outline
        // (-> rendering the quad fully filled as outline color)
        // We do the test in atlas-spread space instead of fragment, so we can clamp
        // the ouline to the constant range ]-0.5..0.5[
        float halfOutline_atlasSpread = computeDistanceInAtlasSpread(halfOutline_fragment);
        halfOutline_atlasSpread = clamp(halfOutline_atlasSpread, -0.499, 0.499);
        if (signedDistance + halfOutline_atlasSpread < 0)
        {
            discard;
        }
        else if (signedDistance - halfOutline_atlasSpread <= 0)
        {
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
        float distance_fragment = computeDistanceInFragmentUnit(signedDistance);
        if(distance_fragment <= 0)
        {
            // The outline fade-in zone: fades between the colorbuffer and outline-color

            #define CLAMP_OUTLINE
            #ifdef CLAMP_OUTLINE
                float maxOutline_fragment = computeDistanceInFragmentUnit(0.5);
                // God showed me to substract 0.5 while I was pooping.
                // The coverage formula is:
                // 0.5 + distance_fragment + halfOutline_fragment
                // The maximal distance_fragment is computeDistanceInFragmentUnit(-0.5) == -K
                // To allow the formula to equal zero at max distance, we must ensure
                // 0.5 + -K + halfOutline_fragment = 0
                // halfOutline_framgnet = +K - 0.5 == computeDistanceInFragmentUnit(+0.5) - 0.5.
                float halfOutline_fragment = min(halfOutline_fragment, maxOutline_fragment - 0.5);
            #else
                float halfOutline_fragment = halfOutline_fragment;
            #endif

            float coverage = clamp(0.5 + distance_fragment + halfOutline_fragment, 0, 1);
            out_Color = vec4(OUTLINE_COLOR, coverage * ex_Color.a);
        }
        else
        {
            // The outline solid + fad-out zone: fades between outline-color and text-color
            //#define OUTLINE_ONLY
            #ifdef OUTLINE_ONLY
                float coverage = clamp(0.5 + halfOutline_fragment - distance_fragment, 0, 1);
                out_Color = vec4(OUTLINE_COLOR, coverage * ex_Color.a);
            #else
                float coverage = 0.5 + halfOutline_fragment - distance_fragment;
                vec3 albedo = mix(ex_Color.rgb, OUTLINE_COLOR, coverage);
                out_Color = vec4(albedo, ex_Color.a);
            #endif
        }
    #else // !(OUTLINE_TEXT)
        // Smoothing, without oultine
        out_Color = vec4(ex_Color.rgb,
                         ex_Color.a * computeFragmentCoverage(signedDistance));
    #endif //OUTLINE_TEXT
#endif // SMOOTH_TEXT

    out_Color = correctGamma(out_Color);
}