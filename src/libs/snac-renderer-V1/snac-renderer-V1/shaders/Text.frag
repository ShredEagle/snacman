#version 420

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
