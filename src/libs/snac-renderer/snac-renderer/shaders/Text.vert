#version 420

// TODO This should not be hardcoded, but somehow provided by client code
#define GlyphMetricsLength 106

layout(location=0) in vec2 ve_Position_u; // expect a quad, from [0, -1] to [1, 0]
layout(location=1) in vec2 ve_TextureCoords0_u;

layout(location= 4) in vec2  in_InstancePosition_pix;
layout(location= 5) in mat4  in_LocalToWorld_pixToWorldUnit;
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
    vec4 stringOrigin_world = in_LocalToWorld_pixToWorldUnit[3];
    vec4 stringOrigin_clip = u_ViewingMatrix * stringOrigin_world;
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
        * vec4(
            (in_LocalToWorld_pixToWorldUnit * vec4(vertexPositionInString_pix, 0., 1.)).xy / (u_FramebufferResolution_p/2),
            0.,
            1.
        );
#endif

    ex_Scale = sqrt(in_LocalToWorld_pixToWorldUnit[0][0] * in_LocalToWorld_pixToWorldUnit[0][0]
                    + in_LocalToWorld_pixToWorldUnit[0][1] * in_LocalToWorld_pixToWorldUnit[0][1]);
    ex_AtlasCoords = textureOffset + (ve_TextureCoords0_u * boundingBox);
    ex_Albedo = in_Albedo;
}
