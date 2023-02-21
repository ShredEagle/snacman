#pragma once


#include "../Mesh.h"

#include <arte/Freetype.h>

#include <graphics/detail/GlyphUtilities.h>

#include <math/Homogeneous.h>

#include <resource/ResourceFinder.h>
#include <renderer/ShaderSource.h>


namespace ad {
namespace snac {


/// @brief  Data structure to populate the instances' vertex buffer.
struct GlyphInstance
{
    math::AffineMatrix<3, GLfloat> glyphToScreen_p;
    math::sdr::Rgba albedo;
    GLint offsetInTexture_p; // horizontal offset to the glyph in its ribbon texture.
    math::Size<2, GLfloat> boundingBox_p; // glyph bounding box in texture pixel coordinates, including margins.
    math::Vec<2, GLfloat> bearing_p; // bearing, including  margins.
};


InstanceStream initializeGlyphInstanceStream();

/// @brief Groups all the data required to prepare a string for rendering (i.e. constructing the GlyphInstances)
struct GlyphAtlas
{
    GlyphAtlas(arte::FontFace aFontFace);

    // TODO #text
    // This is the most problematic point currently, because GlyphInstance mixes together data
    // that will be constant (per string) and data that should be kept dynamic.
    // Ideally, we would pre-compute each string once during initialization, this data is per glyph i.e:
    // * glyph atlas offset, bouding box and bearing
    // * glyph position relative to the string origin
    // Then, dynamic data could be somehow added. This data is likely per string i.e.:
    // * string pose 
    // * string color
    std::vector<GlyphInstance> populateInstances(const std::string & aString,
                                                 math::sdr::Rgba aColor,
                                                 math::AffineMatrix<3, float> aLocalToScreen_p) const;

    arte::FontFace mFontFace;
    graphics::detail::StaticGlyphCache mGlyphCache;
};


Mesh makeGlyphMesh(GlyphAtlas & aGlyphAtlas, std::shared_ptr<Effect> aEffect);

/// @brief This high level structure is intended to be the resource representing a loaded font (at a given size).
/// Its instances are intended to be shared between entities using this font.
struct Font
{
    Font(arte::FontFace aFontFace,
         unsigned int aFontPixelHeight,
         std::shared_ptr<Effect> aEffect) :
        mGlyphAtlas{
            std::move(aFontFace
                        .setPixelHeight(aFontPixelHeight)
                        .inverseYAxis(true))
        },
        mGlyphMesh{makeGlyphMesh(mGlyphAtlas, std::move(aEffect))}
    {}

    GlyphAtlas mGlyphAtlas;
    Mesh mGlyphMesh;
};




} // namespace snac
} // namespace ad