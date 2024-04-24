in uvec4 ve_Joints0;
in vec4  ve_Weights0;

in uint in_MatrixPaletteOffset; // default value of 0 is what we need when a single palette is provided.

// Having a fixed binding value in an included file is dangerous.
// Yet this seems to be required to avoid collisions with other uniform blocks.
layout(std140, binding = 3) uniform JointMatricesBlock
{
    mat4 joints[512];
};

mat4 assembleSkinningMatrix()
{
    return 
          ve_Weights0[0] * joints[in_MatrixPaletteOffset + ve_Joints0[0]]
        + ve_Weights0[1] * joints[in_MatrixPaletteOffset + ve_Joints0[1]]
        + ve_Weights0[2] * joints[in_MatrixPaletteOffset + ve_Joints0[2]]
        + ve_Weights0[3] * joints[in_MatrixPaletteOffset + ve_Joints0[3]]
    ;
}