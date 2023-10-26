#pragma once

#include <math/Color.h>


namespace ad::renderer {


struct TextureInput
{
    using Index = unsigned int;
    inline static constexpr Index gNoEntry = std::numeric_limits<Index>::max();

    Index mTextureIndex = gNoEntry; // Index of the texture in storage. Maybe could be the texture name directly.
    Index mUVAttributeIndex = gNoEntry;
};


// Note: 16-aligned, because it is intended to be stored as an array in a buffer object
// and then the elements are accessed via a std140 uniform block 
struct alignas(16) PhongMaterial 
{
    // 4 components, so it can be loaded directly on the GPU without alignment issues.
    math::hdr::Rgba<float> mAmbientColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mDiffuseColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mSpecularColor = math::hdr::gWhite<float>;
    TextureInput mDiffuseMap; 
    float mSpecularExponent = 1.f;
};


// Makes sure the total reflected "power" with a pure white light (1, 1, 1) does not exceed 1
inline void normalizeColorFactors(PhongMaterial & aMaterial)
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