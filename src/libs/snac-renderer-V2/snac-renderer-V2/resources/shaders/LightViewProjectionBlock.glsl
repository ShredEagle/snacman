#if !defined(LIGHTVIEWPROJECTIONBLOCK_GLSL_INCLUDE_GUARD)
#define LIGHTVIEWPROJECTIONBLOCK_GLSL_INCLUDE_GUARD


#include "Constants.glsl"

layout(std140, binding = 5) uniform LightViewProjectionBlock
{
    uint lightViewProjectionCount;
    mat4 lightViewProjections[MAX_SHADOW_MAPS];
};


#endif // include guard