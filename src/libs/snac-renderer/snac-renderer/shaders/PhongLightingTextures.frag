#version 420

in vec3 ex_Position_v;
in vec3 ex_Normal_v;
in vec2[2] ex_TextureCoords;
in vec4 ex_ColorFactor;

uniform sampler2D u_BaseColorTexture;

uniform vec3 u_LightPosition_v;
uniform vec3 u_LightColor = vec3(0.8, 0.0, 0.8);
uniform vec3 u_AmbientColor = vec3(0.2, 0.0, 0.2);
uniform uint u_BaseColorUVIndex;

out vec4 out_Color;

void main(void)
{
    // Everything in camera space
    vec3 light = normalize(u_LightPosition_v - ex_Position_v);
    vec3 view = vec3(0., 0., 1.);
    vec3 h = normalize(view + light);
    vec3 normal = normalize(ex_Normal_v); // cannot normalize in vertex shader, as interpolation changes length.
    
    float specularExponent = 32;

    vec4 color = 
        ex_ColorFactor 
        * texture(u_BaseColorTexture, ex_TextureCoords[u_BaseColorUVIndex]);

    vec3 diffuse = 
        color.xyz * (u_AmbientColor + u_LightColor * max(0.f, dot(normal, light)));
    vec3 specular = 
        u_LightColor * color.xyz * pow(max(0.f, dot(normal, h)), specularExponent);
    vec3 phongColor = diffuse + specular;

    // Gamma correction
    float gamma = 2.2;
    // Gamma compression from linear color space to "simple sRGB", as expected by the monitor.
    // (The monitor will do the gamma expansion.)
    out_Color = vec4(pow(phongColor, vec3(1./gamma)), color.w);
}