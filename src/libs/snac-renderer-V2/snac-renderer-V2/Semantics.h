#pragma once


namespace ad::renderer {


namespace semantic
{
    #define SEM(s) const Semantic g ## s{#s}

    const Semantic g_builtin = "_builtin";
    const Semantic gPosition{"Position"};
    SEM(ModelTransform);
    const Semantic gNormal{"Normal"};
    const Semantic gTangent{"Tangent"};
    SEM(Bitangent);
    const Semantic gColor{"Color"};
    const Semantic gUv{"Uv"};
    SEM(GlyphAtlas);
    SEM(GlyphIdx);
    SEM(ShadowMap);
    SEM(Joints0);
    SEM(Weights0);
    const Semantic gDiffuseTexture{"DiffuseTexture"};
    const Semantic gNormalTexture{"NormalTexture"};
    SEM(MetallicRoughnessAoTexture);
    SEM(EnvironmentTexture);
    SEM(FilteredRadianceEnvironmentTexture);
    SEM(IntegratedEnvironmentBrdf);
    SEM(FilteredIrradianceEnvironmentTexture);
    SEM(EntityIdx);
    const Semantic gModelTransformIdx{"ModelTransformIdx"};
    const Semantic gMaterialIdx{"MaterialIdx"};
    SEM(MatrixPaletteOffset);

    #undef SEM

    #define BLOCK_SEM(s) const BlockSemantic g ## s{#s}

    BLOCK_SEM(Entities);
    BLOCK_SEM(TextEntities);
    const BlockSemantic gFrame{"Frame"};
    BLOCK_SEM(GlyphMetrics);
    BLOCK_SEM(JointMatrices);
    BLOCK_SEM(Lights);
    BLOCK_SEM(LightViewProjection);
    const BlockSemantic gLocalToWorld{"LocalToWorld"};
    const BlockSemantic gMaterials{"Materials"};
    BLOCK_SEM(ShadowCascade);
    BLOCK_SEM(ViewProjection);

    #undef BLOCK_SEM

} // namespace semantic


} // namespace ad::renderer