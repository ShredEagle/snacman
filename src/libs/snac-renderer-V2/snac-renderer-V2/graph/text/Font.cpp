// TODO Ad 2024/11/14: 
// * Render text in Viewer
//   * Move text into scene
//   * Provide constant values via defines or something similar
//   * Ensure several fonts can be stored in the same texture (array?) and do a single draw call
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
    mFontFace{std::move(aFontFace)},
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


std::vector<GlyphInstanceData> prepareText(const Font & aFont, const std::string & aString)
{
    std::vector<GlyphInstanceData> result;
    result.reserve(aString.size());

    graphics::PenPosition penPosition;
    for (std::string::const_iterator it = aString.begin();
         it != aString.end();
         /* in body */)
    {
        // Decode utf8 encoded string to individual Unicode code points,
        // and advance the iterator accordingly.
        arte::CharCode codePoint = utf8::next(it, aString.end());
        assert(codePoint >= aFont.mFirstChar 
               && codePoint < aFont.mFirstChar + aFont.mCharMap.size());

        GLuint glyphIndex = codePoint - aFont.mFirstChar;
        const GlyphData::FreeTypeData & ftData = aFont.mCharMap.mFt[glyphIndex];

        result.push_back(GlyphInstanceData{
            .mGlyphIdx = glyphIndex,
            .mPosition_stringPix = 
                penPosition.advance(ftData.mPenAdvance, ftData.mFreetypeIndex, aFont.mFontFace)
        });
    }
    return result;
}


} // namespace ad::renderer