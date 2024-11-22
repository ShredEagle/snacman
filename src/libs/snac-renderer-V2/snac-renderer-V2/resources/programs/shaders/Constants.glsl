#if !defined(CONSTANTS_GLSL_INCLUDE_GUARD)
#define CONSTANTS_GLSL_INCLUDE_GUARD


#define INVALID_INDEX uint(-1)

////
// Define values from client
////

// Note: we could use the defines provided by the client directly in the shader code
// but I would rather have the definition visible in some GLSL code to be grep friendly.

#define SDF_DOUBLE_SPREAD CLIENT_SDF_DOUBLE_SPREAD

// TODO ideally, those constants would be replaced by defines provided by CPP
#define MAX_ENTITIES 512
#define MAX_LIGHTS 16
#define MAX_SHADOWS 4 // The maximum number of lights that could project shadows
#define CASCADES_PER_SHADOW 4
#define MAX_SHADOW_MAPS (MAX_SHADOWS * CASCADES_PER_SHADOW)

#endif // include guard