#version 420

layout(location=0) in vec2 ve_Position_u; // expect a quad, from [0, -1] to [1, 0]
layout(location=1) in vec2 ve_TextureCoords0_u;

layout(location= 4) in mat3  in_LocalToWorld_glyphToScreenPixels;
layout(location= 8) in vec4  in_Albedo;
layout(location= 9) in uint in_GlyphIndex;

uniform ivec2 in_FramebufferResolution_p;


struct Metrics 
{
  vec2 bounding;
  vec2 bearing;
  uint linearOffset;
};

layout(std140) uniform GlyphMetricsBlock
{
    Metrics glyphs[106];
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
    vec2 vertexPositionInGlyph_p = (ve_Position_u * boundingBox) + bearing;

    gl_Position = vec4(
        (in_LocalToWorld_glyphToScreenPixels * vec3(vertexPositionInGlyph_p, 1.)).xy / in_FramebufferResolution_p,
        0.,
        1.
    );

    ex_Scale = sqrt(in_LocalToWorld_glyphToScreenPixels[0][0] * in_LocalToWorld_glyphToScreenPixels[0][0]
    + in_LocalToWorld_glyphToScreenPixels[0][1] * in_LocalToWorld_glyphToScreenPixels[0][1]);
    ex_AtlasCoords = textureOffset + (ve_TextureCoords0_u * boundingBox);
    ex_Albedo = in_Albedo;
}
