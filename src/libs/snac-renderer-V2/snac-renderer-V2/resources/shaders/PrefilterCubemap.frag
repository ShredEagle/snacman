#version 420

uniform samplerCube u_SkyboxTexture;

layout(location = 0) out vec3 out_LinearHdr;

void main()
{
    out_LinearHdr = vec3(0.5);
}
