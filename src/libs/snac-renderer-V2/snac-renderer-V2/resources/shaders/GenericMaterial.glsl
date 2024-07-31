struct GenericMaterial
{
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    uint diffuseTextureIndex;
    uint diffuseUvChannel;
    uint normalTextureIndex;
    uint normalUvChannel;
    uint mraoTextureIndex;
    uint mraoUvChannel;
    float specularExponent;
};

layout(std140, binding = 2) uniform MaterialsBlock
{
    GenericMaterial ub_MaterialParams[128];
};
