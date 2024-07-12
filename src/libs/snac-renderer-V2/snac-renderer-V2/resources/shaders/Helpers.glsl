#if !defined(HELPERS_GLSL_INCLUDE_GUARD)
#define HELPERS_GLSL_INCLUDE_GUARD


const float M_PI = 3.141592653589793;


float dotPlus(vec3 a, vec3 b)
{
    return max(0.f, dot(a, b));
}


float maxCw(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}


float maxCw(vec4 v)
{
    //see: https://stackoverflow.com/a/77071476
    vec2 pairs = max(v.xy, v.zw);
    return max(pairs.x, pairs.y);
}


// TODO find a better name for this operation, and its symmetrical.
// Remaps a vector [-magniture, magnitude]^3 to [0, 1]^3.
// Notably useful to display unit direction vectors as colors.
vec3 remapToRgb(vec3 aInput, float aMagnitude = 1)
{
    return (aInput + vec3(aMagnitude)) / (2 * aMagnitude);
}


/// @param aWorldDirection direction in world space, expected in the usual right-handed world basis.
vec3 worldToCubemap(vec3 aWorldDirection)
{
    // The cubemap basis is left-handed
    // see: https://www.khronos.org/opengl/wiki/Cubemap_Texture
    return vec3(aWorldDirection.xy, -aWorldDirection.z);
}


#endif //include guard