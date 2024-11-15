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


namespace ad::renderer {


Font::Font(arte::FontFace aFontFace,
           int aFontPixelHeight,
           Storage & aStorage,
           arte::CharCode aFirstChar,
           arte::CharCode aLastChar) :
    mFirstChar{aFirstChar}
{
    assert(aLastChar >= aFirstChar);
    mCharMap.reserve(aLastChar - aFirstChar);

    aFontFace.setPixelHeight(aFontPixelHeight)
             .inverseYAxis(true);

    graphics::Texture atlas = graphics::makeTightGlyphAtlas(
        aFontFace,
        aFirstChar,
        aLastChar,
        [this](arte::CharCode aCode, const graphics::RenderedGlyph & aGlyph)
        {
            mCharMap.push_back(aGlyph);
        },
        {1, 1}
    );
    // TODO: should be more streamlined with ribbon creation
    aStorage.mTextures.push_back(std::move(atlas));
    mGlyphAtlas = &aStorage.mTextures.back();
    glObjectLabel(GL_TEXTURE, *mGlyphAtlas, -1, "Font_Atlas");
}


} // namespace ad::renderer