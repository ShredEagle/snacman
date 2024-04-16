#pragma once


namespace ad::renderer {


namespace semantic
{
    #define SEM(s) const Semantic g ## s{#s}

    const Semantic gPosition{"Position"};
    SEM(ModelTransform);
    const Semantic gNormal{"Normal"};
    const Semantic gTangent{"Tangent"};
    const Semantic gColor{"Color"};
    const Semantic gUv{"Uv"};
    SEM(Joints0);
    SEM(Weights0);
    const Semantic gDiffuseTexture{"DiffuseTexture"};
    const Semantic gNormalTexture{"NormalTexture"};
    const Semantic gModelTransformIdx{"ModelTransformIdx"};
    const Semantic gMaterialIdx{"MaterialIdx"};
    SEM(MatrixPaletteOffset);

    #undef SEM

    #define BLOCK_SEM(s) const BlockSemantic g ## s{#s}

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
    const BlockSemantic gMaterials{"Materials"};
    const BlockSemantic gLocalToWorld{"LocalToWorld"};
    BLOCK_SEM(JointMatrices);

    #undef BLOCK_SEM

} // namespace semantic


} // namespace ad::renderer