#pragma once

#include <math/Color.h>


namespace ad::renderer {


struct TextureInput
{
    using Index = unsigned int;
    inline static constexpr Index gNoEntry = std::numeric_limits<Index>::max();

    // NOTE Ad 2024/02/21: If one day we go with bindless textures, this should probably become
    //   the "address" of a plain TEXTURE_2D 
    //   (since we would not have to assemble several logical textures in a common TEXTURE_2D_ARRAY)
    Index mTextureIndex = gNoEntry; // Index of the texture in the TEXTURE_2D_ARRAY.

    Index mUVAttributeIndex = gNoEntry;
};


// Note: 16-aligned, because it is intended to be stored as an array in a buffer object
// and then the elements are accessed via a std140 uniform block 
/// \brief Attempts to cover both Phong and Pbr materials, to simplify development
struct alignas(16) GenericMaterial 
{
    // 4 components, so it can be loaded directly on the GPU without alignment issues.
    math::hdr::Rgba<float> mAmbientColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mDiffuseColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mSpecularColor = math::hdr::gWhite<float>;
    TextureInput mDiffuseMap; 
    TextureInput mNormalMap; 
    TextureInput mMetallicRoughnessAoMap; 
    float mSpecularExponent = 1.f;
};


// Makes sure the total reflected "power" with a pure white light (1, 1, 1) does not exceed 1
inline void normalizeColorFactors(GenericMaterial & aMaterial)
{
    auto sum = aMaterial.mAmbientColor + aMaterial.mDiffuseColor + aMaterial.mSpecularColor;
    auto factor = sum.getNorm();

    auto divide = [](auto & aColor, float aFactor)
    {
        auto alpha = aColor.a();
        aColor /= aFactor;
        aColor.a() = alpha;
    };

    if (factor > 1)
    {
        factor /= std::sqrt(3.f);
        divide(aMaterial.mAmbientColor, factor);
        divide(aMaterial.mDiffuseColor, factor);
        divide(aMaterial.mSpecularColor, factor);
    }
}

} // namespace ad::renderer