in uvec4 ve_Joints0;
in vec4  ve_Weights0;

// Having a fixed binding value in an included file is dangerous.
// Yet this seems to be required to avoid collisions with other uniform blocks.
layout(std140, binding = 1) uniform JointMatricesBlock
{
    mat4 joints[512];
};

mat4 assembleSkinningMatrix(uint matrixPaletteOffset)
{
    return 
          ve_Weights0[0] * joints[matrixPaletteOffset + ve_Joints0[0]]
        + ve_Weights0[1] * joints[matrixPaletteOffset + ve_Joints0[1]]
        + ve_Weights0[2] * joints[matrixPaletteOffset + ve_Joints0[2]]
        + ve_Weights0[3] * joints[matrixPaletteOffset + ve_Joints0[3]]
    ;
}