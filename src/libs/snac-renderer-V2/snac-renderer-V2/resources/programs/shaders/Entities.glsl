#if !defined(ENTITIES_GLSL_INCLUDE_GUARD)
#define ENTITIES_GLSL_INCLUDE_GUARD


// The instance attribute associating the OpenGL instance
// (in the sense of instanced rendering) to its corresponding Entity.
//in uint in_EntityIdx;

struct EntityData
{
    mat4 localToWorld;
    // Will be required if we were to support non-uniform scaling.
    //layout(location=10) in mat4 in_LocalToWorldInverseTranspose;
    vec4 colorFactor;

    // Note: There is an alternative regarding MatrixPaletteOffset,
    // which could be either an instance (vertex) attribute,
    //   * The current appraoch
    // or an EntityData member.
    //   * Having it as a data member even if RIGGING is not defined is a drawback of the second appraoch.
};

// TODO #ssbo Use a shader storage block, due to the unbounded nature of the number of instances
layout(std140, binding = 1) uniform EntitiesBlock
{
    EntityData ub_Entities[MAX_ENTITIES];
};

mat4 getModelTransform()
{
    return ub_Entities[ENTITY_IDX_ATTRIBUTE].localToWorld;
}

EntityData getEntity()
{
    return ub_Entities[ENTITY_IDX_ATTRIBUTE];
}


#endif //ENTITIES_GLSL_INCLUDE_GUARD