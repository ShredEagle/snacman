#pragma once


#include "../Mesh.h"

#include <arte/Freetype.h>

#include <graphics/TextUtilities.h>

#include <math/Homogeneous.h>

#include <resource/ResourceFinder.h>
#include <renderer/ShaderSource.h>


namespace ad {
namespace snac {


/// @brief All the metrics required to render a single glyph in combination with a glyph atlas.
/// Intended to be uploaded as an array (e.g. uniform block) to the GPU.
struct GlyphMetrics
{
    math::Size<2, GLfloat> boundingBox_p; // glyph bounding box in texture pixel coordinates, including margins.
    math::Vec<2, GLfloat> bearing_p; // bearing, including  margins.
    GLuint offsetInTexture_p; // horizontal offset to the glyph in its ribbon texture.
    math::Vec<3, GLuint> _padding; // layout std140 imposes that array are 16 bytes aligned
};


// TODO Ideally, the string's transformation matrix would not be stored, only a bidimensionnal local pen advance.
/// @brief The per-instance data for instanced rendering of text.
struct GlyphInstance
{
    // This transformation is composed of the pen advance plus the whole string transformation (i.e. the same for all glyphs in a given string).
    // We do not have an abstraction of the whole string at the shader level for the moment.
    math::AffineMatrix<3, GLfloat> glyphToScreen_p;
    math::sdr::Rgba albedo;
    GLuint entryIndex; // Index into an array of GlyphMetrics
};

    
/// @brief Mapping character code point to their corresponding graphics::RenderedGlyph.
struct GlyphMap
{
    const graphics::RenderedGlyph & at(arte::CharCode aCharCode) const;
    // Useful as a GlyphCallback
    void insert(arte::CharCode aCharCode, const graphics::RenderedGlyph & aGlyph);

    // TODO does not need to store the RenderedGlyph entirely, only the metrics required to populateInstance().
    std::unordered_map<arte::CharCode, graphics::RenderedGlyph> mCharCodeToGlyph;
    static constexpr arte::CharCode placeholder = 0x3F; // '?'
};


// TODO #text could become the Font struct, and the mesh would be hosted by the text renderer directly?
// The issue with that is that the mesh keeps the material, which keeps the texture. So we need 1 mesh / fontface atm.
/// @brief Groups all the data required to prepare a string for rendering (i.e. constructing the GlyphInstances)
struct FontData
{
    FontData(arte::FontFace aFontFace);

    // TODO #text
    // This is the most problematic point currently, because GlyphInstance mixes together data
    // that will be constant (per string) and data that should be kept dynamic.
    // Ideally, we would pre-compute each string once during initialization, this data is per glyph i.e:
    // * glyph atlas offset, bouding box and bearing
    // * glyph position relative to the string origin
    // Then, dynamic data could be somehow added. This data is likely per string i.e.:
    // * string pose 
    // * string color
    /// @param aStringLocalToScreen_p Transformation from the string local space **in pixel** to screen space, also in pixel.
    std::vector<GlyphInstance> populateInstances(const std::string & aString,
                                                 math::sdr::Rgba aColor,
                                                 math::AffineMatrix<3, float> aStringLocalToScreen_p) const;

    arte::FontFace mFontFace;
    GlyphMap mGlyphMap;
    // After the glyphmap, which will be written by the atlas initializer
    std::shared_ptr<graphics::Texture> mGlyphAtlas;
};


Mesh makeGlyphMesh(FontData & aFontData, std::shared_ptr<Effect> aEffect);


/// @brief This high level structure is intended to be the resource representing a loaded font (at a given size).
/// Its instances are intended to be shared between entities using this font.
struct Font
{
    Font(arte::FontFace aFontFace,
         unsigned int aFontPixelHeight,
         std::shared_ptr<Effect> aEffect) :
        mFontData{
            std::move(aFontFace
                        .setPixelHeight(aFontPixelHeight)
                        .inverseYAxis(true))
        },
        mGlyphMesh{makeGlyphMesh(mFontData, std::move(aEffect))}
    {}

    FontData mFontData;
    Mesh mGlyphMesh;
};




} // namespace snac
} // namespace ad
