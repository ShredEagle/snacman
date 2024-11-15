#pragma once

#include "../../Handle.h"

#include <arte/Freetype.h>

#include <vector>

// TODO Ad 2024/11/14: Remove this include
#include <graphics/TextUtilities.h>

namespace ad::renderer {


struct Storage;


constexpr arte::CharCode gFirstCharCode = 20;
constexpr arte::CharCode gLastCharCode = 126;

//struct GlyphData
//{
//
//};
// TODO define a custom type
using GlyphData = graphics::RenderedGlyph;

struct Font
{
    using CharMap_t = std::vector<GlyphData>;

    // Add a ctor that loads form cache file
    Font(arte::FontFace aFontFace,
         int aFontPixelHeight,
         Storage & aStorage,
         arte::CharCode aFirstChar = gFirstCharCode,
         arte::CharCode aLastChar = gLastCharCode);

    arte::CharCode mFirstChar;
    CharMap_t mCharMap;
    renderer::Handle<graphics::Texture> mGlyphAtlas;
};


} // namespace ad::renderer