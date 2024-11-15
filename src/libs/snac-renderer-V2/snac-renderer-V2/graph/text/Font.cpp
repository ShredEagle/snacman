// TODO Ad 2024/11/14: 
// * Render text in Viewer
//   * Move text into scene
//   * Provide constant values via defines or something similar
// * Implement cache files
// * Name font atlas textures
// * Allow to load from a loader interface (for render thread / cache handling)
// * Decomissin renderer_V1 completely
#include "Font.h"

#include "../../Model.h"

// TODO Ad 2024/11/14: Remove this include
#include <graphics/TextUtilities.h>

#include <type_traits>


namespace ad::renderer {


namespace {

    GlyphMetrics_glsl toGlyphMetrics(const graphics::RenderedGlyph & aGlyph)
    {
        static_assert(std::is_same_v<decltype(aGlyph.offsetInTexture), GLuint>,
            "If the offset become 2 dimensionnal, the code below must be updated.");

        return GlyphMetrics_glsl{
            .mBoundingBox_pix = aGlyph.controlBoxSize,
            .mBearing_pix = aGlyph.bearing,
            .mOffsetInTexture_pix = {aGlyph.offsetInTexture, 0u},
        };
    }

} // unnamed namespace


void GlyphData::push(const graphics::RenderedGlyph & aGlyph)
{
    mMetrics.push_back(toGlyphMetrics(aGlyph));
    mFt.push_back(FreeTypeData{
        .mPenAdvance = aGlyph.penAdvance,
        .mFreetypeIndex = aGlyph.freetypeIndex,
    });
}


Font::Font(arte::FontFace aFontFace,
           int aFontPixelHeight,
           Storage & aStorage,
           arte::CharCode aFirstChar,
           arte::CharCode aLastChar) :
    mFirstChar{aFirstChar},
    mCharMap{aLastChar - aFirstChar}
{
    assert(aLastChar >= aFirstChar);

    aFontFace.setPixelHeight(aFontPixelHeight)
             .inverseYAxis(true);

    graphics::Texture atlas = graphics::makeTightGlyphAtlas(
        aFontFace,
        aFirstChar,
        aLastChar,
        [this](arte::CharCode aCode, const graphics::RenderedGlyph & aGlyph)
        {
            mCharMap.push(aGlyph);
        },
        {1, 1}
    );
    // TODO: should be more streamlined with ribbon creation
    aStorage.mTextures.push_back(std::move(atlas));
    mGlyphAtlas = &aStorage.mTextures.back();
    glObjectLabel(GL_TEXTURE, *mGlyphAtlas, -1, "Font_Atlas");
}


} // namespace ad::renderer