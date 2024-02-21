#pragma once


namespace ad::renderer {


namespace semantic
{
    const Semantic gPosition{"Position"};
    const Semantic gNormal{"Normal"};
    const Semantic gTangent{"Tangent"};
    const Semantic gColor{"Color"};
    const Semantic gUv{"Uv"};
    const Semantic gDiffuseTexture{"DiffuseTexture"};
    const Semantic gNormalTexture{"NormalTexture"};
    const Semantic gModelTransformIdx{"ModelTransformIdx"};
    const Semantic gMaterialIdx{"MaterialIdx"};

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
    const BlockSemantic gMaterials{"Materials"};
    const BlockSemantic gLocalToWorld{"LocalToWorld"};
} // namespace semantic


} // namespace ad::renderer