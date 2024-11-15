#pragma once

#include "TextGlsl.h"

#include "../../Handle.h"

#include <arte/Freetype.h>

#include <vector>

// TODO Ad 2024/11/14: Remove this include
#include <graphics/TextUtilities.h>

namespace ad::renderer {


struct Storage;


constexpr arte::CharCode gFirstCharCode = 32;
constexpr arte::CharCode gLastCharCode = 126;


/// @brief SoA for per-glyph information. Can be used as a character map
/// if the charcode is used as index.
struct GlyphData
{
    struct FreeTypeData
    {
        math::Vec<2, GLfloat> mPenAdvance;
        unsigned int mFreetypeIndex; // Notably usefull for kerning queries.
    };

    GlyphData(std::size_t aCapacity)
    {
        mMetrics.reserve(aCapacity);
        mFt.reserve(aCapacity);
    }

    std::size_t size() const
    {
        assert(mMetrics.size() == mFt.size());
        return mMetrics.size();
    }

    void push(const graphics::RenderedGlyph & aGlyph);

    /// @brief Data that should be loaded as a GLSL buffer
    std::vector<GlyphMetrics_glsl> mMetrics;
    /// @brief Data required to layout a string as a serie of glyphs
    std::vector<FreeTypeData> mFt;
};


/// @brief Model a font at a given size as a resource.
struct Font
{
    using CharMap_t = GlyphData;

    // Add a ctor that loads form cache file
    Font(arte::FontFace aFontFace,
         int aFontPixelHeight,
         Storage & aStorage,
         arte::CharCode aFirstChar = gFirstCharCode,
         arte::CharCode aLastChar = gLastCharCode);

    // Needed for kerning
    arte::FontFace mFontFace;
    arte::CharCode mFirstChar;
    CharMap_t mCharMap;
    renderer::Handle<graphics::Texture> mGlyphAtlas;
};


std::vector<GlyphInstanceData> prepareText(const Font & aFont, const std::string & aString);


} // namespace ad::renderer