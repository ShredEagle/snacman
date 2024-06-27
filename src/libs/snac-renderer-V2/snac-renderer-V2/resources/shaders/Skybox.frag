#version 420

#include "Gamma.glsl"
#include "Helpers.glsl"

in vec3 ex_CubeTextureCoords;

#if defined(EQUIRECTANGULAR)
uniform sampler2D u_SkyboxTexture;
#else
uniform samplerCube u_SkyboxTexture;
#endif

out vec4 out_Color;


void main()
{
#if defined(EQUIRECTANGULAR)
    // For the conversion procedure, see: rtr 4th p407
    vec3 view_world = normalize(ex_CubeTextureCoords);
    vec2 uv = vec2(
        atan(view_world.x, -view_world.z) / (2 * M_PI),
        acos(view_world.y) / M_PI
    );
    vec3 envColor = texture(u_SkyboxTexture, uv).rgb;
#else
    vec3 envColor = texture(u_SkyboxTexture, ex_CubeTextureCoords).rgb;
#endif
    // What is this tone mapping found in Learn OpenGL?
    //envColor = envColor / (envColor + 1.0);
    out_Color = correctGamma(vec4(envColor, 1.0));
}
