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


// TODO Redesign, the intent is to get something that can be loaded at once into a buffer (as an array of)
// but currently the texture index is here, which is of no use from within shader code.
// Maybe it should be split in two classes?
struct PhongMaterial 
{
    // 4 components, so it can be loaded directly on the GPU without alignment issues.
    math::hdr::Rgba<float> mAmbientColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mDiffuseColor  = math::hdr::gWhite<float>;
    math::hdr::Rgba<float> mSpecularColor = math::hdr::gWhite<float>;
    TextureInput mDiffuseMap; 
};


} // namespace ad::renderer