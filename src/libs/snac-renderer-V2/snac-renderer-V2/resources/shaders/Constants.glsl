#if !defined(CONSTANTS_GLSL_INCLUDE_GUARD)
#define CONSTANTS_GLSL_INCLUDE_GUARD

// TODO ideally, those constants would be replaced by defines provided by CPP
#define MAX_LIGHTS 16
#define MAX_SHADOWS 4 // The maximum number of lights that could project shadows
#define CASCADES_PER_SHADOW 4
#define MAX_SHADOW_MAPS (MAX_SHADOWS * CASCADES_PER_SHADOW)

#endif