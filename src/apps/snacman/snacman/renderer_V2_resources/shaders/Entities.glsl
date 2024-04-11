in uint in_EntityIdx;
in uint in_MaterialIdx;


struct EntityData
{
    mat4 localToWorld;
    // Will be required if we were to support non-uniform scaling.
    //layout(location=10) in mat4 in_LocalToWorldInverseTranspose;
    
    vec4 colorFactor;

    // Note: Having this member present even if RIGGING is not defined is a drawback
    // of moving this value from an instance attribute (which could be defined in Rigging.glsl)
    // to a buffer-backed block (which has a more rigid layout).
    uint matrixPaletteOffset;
};


layout(std140, binding = 3) uniform EntitiesBlock
{
    EntityData ub_Entities[512];
};


EntityData getEntity()
{
    return ub_Entities[in_EntityIdx];
}