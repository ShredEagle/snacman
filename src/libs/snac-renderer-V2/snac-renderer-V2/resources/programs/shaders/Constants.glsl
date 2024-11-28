#if !defined(CONSTANTS_GLSL_INCLUDE_GUARD)
#define CONSTANTS_GLSL_INCLUDE_GUARD


#define INVALID_INDEX uint(-1)

////
// Define values from client
////

// Note: we could use the defines provided by the client directly in the shader code
// but I would rather have the definition visible in some GLSL code to be grep friendly.

#define MAX_ENTITIES CLIENT_MAX_ENTITIES
#define MAX_JOINTS CLIENT_MAX_JOINTS
#define SDF_DOUBLE_SPREAD CLIENT_SDF_DOUBLE_SPREAD
#define MAX_LIGHTS CLIENT_MAX_LIGHTS
#define MAX_SHADOW_LIGHTS CLIENT_MAX_SHADOW_LIGHTS // The maximum number of lights that could project shadows
#define CASCADES_PER_SHADOW CLIENT_CASCADES_PER_SHADOW
#define MAX_SHADOW_MAPS (MAX_SHADOW_LIGHTS * CASCADES_PER_SHADOW)

#endif // include guard