#pragma once

#include "TextGlsl.h"

#include "../../Handle.h"

#include <arte/Freetype.h>

#include <vector>

// TODO Ad 2024/11/14: Remove this include
#include <graphics/TextUtilities.h>

namespace ad::renderer {


struct BinaryInArchive;
struct BinaryOutArchive;
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

    explicit GlyphData(std::size_t aCapacity)
    {
        mMetrics.reserve(aCapacity);
        mFt.reserve(aCapacity);
    }

    GlyphData() :
        GlyphData(0)
    {}

    std::size_t size() const
    {
        assert(mMetrics.size() == mFt.size());
        return mMetrics.size();
    }

    void push(const graphics::RenderedGlyph & aGlyph);

    void serialize(BinaryOutArchive & aOut) const;
    void deserialize(BinaryInArchive & aIn);

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
        
    /// @brief Load a Font, using a simple cache system looking for serialized files next to the font path.
    /// Create the cache file if they are not found.
    static Font makeUseCache(const arte::Freetype & aFreetype,
                             std::filesystem::path aFontFullPath,
                             int aFontPixelHeight,
                             Storage & aStorage,
                             arte::CharCode aFirstChar = gFirstCharCode,
                             arte::CharCode aLastChar = gLastCharCode);

    void serialize(BinaryOutArchive & aDataOut, std::ostream & aAtlasOut) const;
    void deserialize(BinaryInArchive & aData, std::istream & aAtlas, Storage & aStorage);

private:
    // Ideally, we would ZII and have a public default ctor
    // but it means we should go over all functions an make sure they just do nothing
    // on the zero inited Font. 
    // Plus, there is a complication that the FontFace data member has no default ctor
    // and adding one would essentially mean having a "null" font face, and null values 
    // are not a good thing in interfaces.
    Font(arte::FontFace aFontFace);
    
public:
    // Needed for kerning
    arte::FontFace mFontFace;
    arte::CharCode mFirstChar;
    CharMap_t mCharMap;
    renderer::Handle<graphics::Texture> mGlyphAtlas;
};


// TODO Ad 2024/11/15: We should actually be able to share a part for several fonts,
// as long as the atlases are accessible from the same texture (e.g. texture array)
// And there is a complication with the UBO of glyph metrics...
// TODO Ad 2024/11/15: Should we also store Parts in Storage to return an handle here?
TextPart makePartForFont(const Font & aFont,
                         Handle<Effect> aEffect,
                         Storage & aStorage);

ClientText prepareText(const Font & aFont, const std::string & aString);


} // namespace ad::renderer