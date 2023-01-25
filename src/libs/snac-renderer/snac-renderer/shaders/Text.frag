#version 420

in vec4 ex_Albedo;
in vec2 ex_AtlasCoords;

out vec4 out_Color;

uniform sampler2DRect u_FontAtlas;

void main(void)
{
    float atlasOpacity = texture(u_FontAtlas, ex_AtlasCoords).r;

    // Gamma correction
    float gamma = 2.2;
    out_Color = vec4(pow(ex_Albedo.rgb, vec3(1./gamma)), ex_Albedo.a * atlasOpacity);
}