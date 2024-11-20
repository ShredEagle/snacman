#version 420

#include "Constants.glsl"
#include "ViewProjectionBlock.glsl"

// TODO This should not be hardcoded, but somehow provided by client code
#define GlyphMetricsLength 106

const vec2 gPositions[4] = vec2[4](
    vec2(0, -1),
    vec2(1, -1),
    vec2(0,  0),
    vec2(1,  0)
);


const vec2 gUvs[4] = vec2[4](
    vec2(0, 0),
    vec2(1, 0),
    vec2(0, 1),
    vec2(1, 1)
);


in uint in_GlyphIdx;
// Position of the pen for this glyph instance, in the overall string
in vec2 in_Position_stringPix;
// The string entity this glyph is part of
in uint in_EntityIdx;


struct GlyphMetrics 
{
  vec2 boundingBox_pix;
  vec2 bearing_pix;
  uint linearOffset_pix;
};

layout(std140, binding = 7) uniform GlyphMetricsBlock
{
    GlyphMetrics glyphs[GlyphMetricsLength];
};


struct StringEntity
{
    mat4 stringPixToWorld;
    vec4 color;
};

layout(std140, binding = 1) uniform EntitiesBlock
{
    StringEntity stringEntities[MAX_ENTITIES];
};


out vec2 ex_AtlasUv_pix;
out vec4 ex_Color;
// Used to adapt the smoothing factor.
out float ex_Scale;

void main(void)
{
    GlyphMetrics glyph = glyphs[in_GlyphIdx];

    ex_AtlasUv_pix = (gUvs[gl_VertexID % 4] * glyph.boundingBox_pix)
        + vec2(glyph.linearOffset_pix, 0);

    StringEntity entity = stringEntities[in_EntityIdx];
    ex_Color = entity.color;

    // Compute the scaling factor applied to the glyph
    // Note: at the moment, we only consider the scaling applied by the model transform
    // and we assume it is uniform.
    // This is likely to evolve, and will raises questions, such as:
    // * should the text outline grow with camera zoom (currently the case)
    mat4 m = entity.stringPixToWorld;
    ex_Scale = sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);

    // Position of this vertex relative to the current pen position in pixels.
    // Note: we consider the glyph coordinate system to go from [0, 0] to [+width, -height]
    // This way, the bearing provided by FreeType can be applied directly.
    // Because bearingY is the Y position of the top of the BBox in the pen coordinate system.
    // see: https://freetype.org/freetype2/docs/glyphs/glyphs-3.html
    vec2 position_penPix = gPositions[gl_VertexID % 4] * glyph.boundingBox_pix + glyph.bearing_pix;
    // Position of this vertex in the overall string coordinate system, in pixels.
    vec2 position_stringPix = in_Position_stringPix + position_penPix;
    // Transform from local string space to clip space
    gl_Position = viewingProjection * entity.stringPixToWorld * vec4(position_stringPix, 0, 1);
}


#if 0
layout(location=0) in vec2 ve_Position_u; // expect a quad, from [0, -1] to [1, 0]
layout(location=1) in vec2 ve_TextureCoords0_u;

layout(location= 4) in vec2  in_InstancePosition_pix;
layout(location= 5) in mat4  in_LocalToWorld;
layout(location= 9) in vec4  in_Albedo;
layout(location= 10) in uint in_GlyphIndex;

uniform ivec2 u_FramebufferResolution_p;


uniform mat4 u_ViewingMatrix;


struct Metrics 
{
  vec2 bounding;
  vec2 bearing;
  uint linearOffset;
};

layout(std140) uniform GlyphMetricsBlock
{
    Metrics glyphs[GlyphMetricsLength];
};

out float ex_Scale;
out vec2 ex_AtlasCoords;
out vec4 ex_Albedo;

void main(void)
{
    // Unpack the glyph entry
    ivec2 textureOffset = ivec2(glyphs[in_GlyphIndex].linearOffset, 0);
    vec2 boundingBox = glyphs[in_GlyphIndex].bounding;
    vec2 bearing = glyphs[in_GlyphIndex].bearing;

    // We consider the glyph coordinate system to go from [0, 0] to [+width, -height]
    // This way, the bearing provided by FreeType can be applied directly.
    vec2 vertexPositionInGlyph_pix = (ve_Position_u * boundingBox) + bearing;

    vec2 vertexPositionInString_pix = vertexPositionInGlyph_pix  + in_InstancePosition_pix;

#ifdef BILLBOARD
    // The last column of the matrix is the translation component.
    vec4 stringOrigin_world = in_LocalToWorld[3];
    vec4 stringOrigin_clip = u_ViewingMatrix * stringOrigin_world;

    // Manually apply perspective division now, so it is not wrongfully applied to the pixel values added later.
    stringOrigin_clip /= stringOrigin_clip.w;

    gl_Position = 
        vec4(
            stringOrigin_clip.xy + (vertexPositionInString_pix / (u_FramebufferResolution_p/2)),
            stringOrigin_clip.z,
            1.
        );
#else
    gl_Position = 
        u_ViewingMatrix
        * in_LocalToWorld 
        * vec4(vertexPositionInString_pix, 0., 1.);
#endif

    // TODO: Franz, please add some comments regarding how this is computed and why this is required : )
    ex_Scale = 
        2 // The division by FB resolution was fixed by introducing a division by 2, the glyphs since appear twice bigger.
        // Take into account the scaling on the summed "modelling" transform
        * sqrt(  in_LocalToWorld[0][0] * in_LocalToWorld[0][0]
                + in_LocalToWorld[0][1] * in_LocalToWorld[0][1])
        // Take into account the scaling on the viewing
        * sqrt(  u_ViewingMatrix[0][0] * u_ViewingMatrix[0][0]
                + u_ViewingMatrix[0][1] * u_ViewingMatrix[0][1])
        // Take into account the effect of the perspective on the scale
        / gl_Position.w;
        ;

    mat4 m = u_ViewingMatrix * in_LocalToWorld;
    ex_Scale = length(vec3(length(m[0].xyz), length(m[1].xyz), length(m[2].xyz))) / gl_Position.w;

    ex_Scale = 2.;

    ex_AtlasCoords = textureOffset + (ve_TextureCoords0_u * boundingBox);
    ex_Albedo = in_Albedo;
}
#endif