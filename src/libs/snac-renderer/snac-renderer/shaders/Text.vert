#version 420

layout(location=1) in vec2 ve_TextureCoords_u;

layout(location= 4) in vec2  in_InstancePosition_u;
layout(location= 8) in vec4  in_Albedo;
//layout(location=10) in vec2  in_Bearing_p;
layout(location= 9) in ivec2 in_TextureOffset_p;
layout(location=10) in vec2  in_BoundingBox_p;

uniform ivec2 in_FramebufferResolution_p;

out vec2 ex_AtlasCoords;
out vec4 ex_Albedo;

void main(void)
{
    vec2 boundingBoxOffset_p = (ve_TextureCoords_u * in_BoundingBox_p);
    gl_Position = vec4(
        in_InstancePosition_u + (/*in_Bearing_p + */boundingBoxOffset_p) / in_FramebufferResolution_p,
        0.,
        1.
    );

    ex_AtlasCoords = in_TextureOffset_p + boundingBoxOffset_p;
    ex_Albedo = in_Albedo;
}