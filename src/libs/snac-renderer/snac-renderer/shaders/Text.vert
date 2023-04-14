#version 420

// TODO This should not be hardcoded, but somehow provided by client code
#define GlyphMetricsLength 106

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
        * vec4(
            (in_LocalToWorld * vec4(vertexPositionInString_pix / (u_FramebufferResolution_p/2), 0., 1.)).xy ,
            0.,
            1.
        );
#endif

    // TODO: Franz, please add some comments regarding how this is computed and why this is required : )
    ex_Scale = 
        2 // The division by FB resolution was fixed by introducing a division by 2, the glyphs since appear twice bigger.
        * sqrt(  in_LocalToWorld[0][0] * in_LocalToWorld[0][0]
               + in_LocalToWorld[0][1] * in_LocalToWorld[0][1]);
    ex_AtlasCoords = textureOffset + (ve_TextureCoords0_u * boundingBox);
    ex_Albedo = in_Albedo;
}
