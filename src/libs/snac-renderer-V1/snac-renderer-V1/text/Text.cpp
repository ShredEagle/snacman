#include "Text.h"

#include "../Cube.h"

#include <renderer/BufferLoad.h>

// TODO remove once not hardcoding a rotation anymore
#include <math/Angle.h>
#include <math/Transformations.h>


namespace ad {
namespace snac {


static_assert(sizeof(GlyphMetrics) % 16 == 0,
                     "GLSL layout std140 requires that array stride is a multiple of 16 bytes.");


constexpr arte::CharCode gFirstCharCode = 20;
constexpr arte::CharCode gLastCharCode = 126;


const graphics::RenderedGlyph & GlyphMap::at(arte::CharCode aCharCode) const
{
    if (auto found = mCharCodeToGlyph.find(aCharCode); found != mCharCodeToGlyph.end())
    {
        return found->second;
    }
    else
    {
        return mCharCodeToGlyph.at(placeholder);
    }
}


void GlyphMap::insert(arte::CharCode aCharCode, const graphics::RenderedGlyph & aGlyph)
{
    mCharCodeToGlyph.emplace(aCharCode, aGlyph);
}


using namespace std::placeholders;


FontData::FontData(arte::FontFace aFontFace) :
    mFontFace{std::move(aFontFace)},
    mGlyphAtlas{
        std::make_shared<graphics::Texture>(
            graphics::makeTightGlyphAtlas(
                mFontFace, 
                gFirstCharCode, 
                gLastCharCode,
                std::bind(&GlyphMap::insert, std::ref(mGlyphMap), _1, _2)))
    }
{}


std::vector<GlyphInstance> FontData::populateInstances(const std::string & aString,
                                                       math::sdr::Rgba aColor,
                                                       math::AffineMatrix<4, float> aStringLocalToWorld) const
{
    std::vector<GlyphInstance> glyphInstances;

    graphics::PenPosition penPosition;
    for (std::string::const_iterator it = aString.begin();
         it != aString.end();
         /* in body */)
    {
        // Decode utf8 encoded string to individual Unicode code points,
        // and advance the iterator accordingly.
        arte::CharCode codePoint = utf8::next(it, aString.end());
        const graphics::RenderedGlyph & glyph = mGlyphMap.at(codePoint);

        glyphInstances.push_back(GlyphInstance{
           .glyphToString_p = penPosition.advance(glyph, mFontFace).as<math::Vec>(),
           .stringToWorld = aStringLocalToWorld,
           .albedo = aColor,
           .entryIndex = codePoint - gFirstCharCode,
        });

        assert(codePoint >= gFirstCharCode && codePoint < gLastCharCode);
    }
        
    return glyphInstances;
}


Mesh makeGlyphMesh(FontData & aFontData, std::shared_ptr<Effect> aEffect)
{
    std::array<GlyphMetrics, gLastCharCode - gFirstCharCode> glyphMetrics;

    for(arte::CharCode codePoint = gFirstCharCode; codePoint != gLastCharCode; ++codePoint)
    {
        const graphics::RenderedGlyph rendered = aFontData.mGlyphMap.at(codePoint);
        glyphMetrics[codePoint - gFirstCharCode] = GlyphMetrics{
            .boundingBox_p = rendered.controlBoxSize,
            .bearing_p = rendered.bearing,
            .offsetInTexture_p = rendered.offsetInTexture,
        };
    }

    auto glyphMetricsBuffer = std::make_shared<graphics::UniformBufferObject>();
    graphics::load(*glyphMetricsBuffer, std::span{glyphMetrics}, graphics::BufferHint::StaticRead);

    auto material = std::make_shared<Material>(Material{
        .mTextures = {{Semantic::FontAtlas, aFontData.mGlyphAtlas}},
        .mUniformBlocks = {{BlockSemantic::GlyphMetrics, std::move(glyphMetricsBuffer)}},
        .mEffect = std::move(aEffect)
    });

    return Mesh{
        .mStream = makeRectangle({ {0.f, -1.f}, {1.f, 1.f}}), // The quad goes from [0, -1] to [1, 0]
        .mMaterial = std::move(material),
    };
}


} // namespace snac
} // namespace ad
