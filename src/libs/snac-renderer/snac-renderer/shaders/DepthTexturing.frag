#version 420

in vec2 ex_TextureCoords;

uniform float u_NearDistance;
uniform float u_FarDistance;

uniform sampler2D u_BaseColorTexture;

out vec4 out_Color;

void main(void)
{
    vec4 color = texture(u_BaseColorTexture, ex_TextureCoords);

    // Normalization of depth
    // see: http://web.archive.org/web/20130416194336/http://olivers.posterous.com/linear-depth-in-glsl-for-real
    // see: http://glampert.com/2014/01-26/visualizing-the-depth-buffer/
    float linearDepth = 
        (2 * u_NearDistance) 
        / (u_FarDistance + u_NearDistance - color.r * (u_FarDistance - u_NearDistance));

    //
    // Gamma correction
    //
    float gamma = 2.2;
    // Gamma compression from linear color space to "simple sRGB", as expected by the monitor.
    // (The monitor will do the gamma expansion.)
    out_Color = vec4(pow(vec3(linearDepth), vec3(1./gamma)), color.w);
}