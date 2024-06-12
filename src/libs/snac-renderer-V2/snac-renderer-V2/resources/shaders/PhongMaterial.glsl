struct PhongMaterial
{
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    uint diffuseTextureIndex;
    uint diffuseUvChannel;
    uint normalTextureIndex;
    uint normalUvChannel;
    float specularExponent;
};

layout(std140, binding = 2) uniform MaterialsBlock
{
    PhongMaterial ub_Phong[128];
};
