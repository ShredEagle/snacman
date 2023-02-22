#version 420

layout(location=0) in vec2 ve_Position_u; // expect a quad, from [0, -1] to [1, 0]
layout(location=1) in vec2 ve_TextureCoords0_u;

layout(location= 4) in mat3  in_LocalToWorld_glyphToScreenPixels;
layout(location= 8) in vec4  in_Albedo;
layout(location= 9) in ivec2 in_TextureOffset_p;
layout(location=10) in vec2  in_BoundingBox_p;
layout(location=11) in vec2  in_Bearing_p;

uniform ivec2 in_FramebufferResolution_p;

out vec2 ex_AtlasCoords;
out vec4 ex_Albedo;

void main(void)
{
    // We consider the glyph coordinate system to go from [0, 0] to [+width, -height]
    // This way, the bearing provided by FreeType can be applied directly.
    vec2 vertexPositionInGlyph_p = (ve_Position_u * in_BoundingBox_p);

    gl_Position = vec4(
        (in_LocalToWorld_glyphToScreenPixels * vec3(in_Bearing_p + vertexPositionInGlyph_p, 1.)).xy / in_FramebufferResolution_p,
        0.,
        1.
    );

    ex_AtlasCoords = in_TextureOffset_p + (ve_TextureCoords0_u * in_BoundingBox_p);
    ex_Albedo = in_Albedo;
}