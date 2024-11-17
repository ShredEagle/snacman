// TODO Ad 2024/11/14: 
// * Render text in Viewer
//   * Restore SDF functionnality
//   * Handle color
//   * Factorize the draw/setup code
//   * Provide constant values via defines or something similar
//   * Ensure several fonts can be stored in the same texture (array?) and do a single draw call
// * Implement cache files
// * Name font atlas textures
// * Allow to load from a loader interface (for render thread / cache handling)
// * Decomission renderer_V1 completely
#include "Font.h"

#include "../../Model.h"
#include "../../Semantics.h"
#include "../../utilities/VertexStreamUtilities.h"

// TODO Ad 2024/11/14: Remove this include
#include <graphics/TextUtilities.h>

// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include <snac-renderer-V2/RendererReimplement.h>

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


ClientText prepareText(const Font & aFont, const std::string & aString)
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


TextPart makePartForFont(const Font & aFont,
                         Handle<Effect> aEffect,
                         Storage & aStorage)
{
    Handle<VertexStream> consolidatedStream = makeVertexStream(
        0,
        0,
        GL_NONE,
        {},
        aStorage,
        makeGlyphInstanceStream(aStorage));

    // Glyph metrics UBO
    aStorage.mUbos.emplace_back();
    Handle<graphics::UniformBufferObject> metricsUbo = &aStorage.mUbos.back();
    proto::load(*metricsUbo, std::span{aFont.mCharMap.mMetrics}, graphics::BufferHint::StaticDraw);

    // String entities UBO
    aStorage.mUbos.emplace_back();
    Handle<graphics::UniformBufferObject> entitiesUbo = &aStorage.mUbos.back();

    aStorage.mMaterialContexts.emplace_back(
        MaterialContext{
            .mUboRepo = RepositoryUbo{
                {semantic::gGlyphMetrics, metricsUbo},
                {semantic::gEntities, entitiesUbo},
            },
            .mTextureRepo = {
                {semantic::gGlyphAtlas, aFont.mGlyphAtlas}
            },
        }
    );
    Handle<MaterialContext> materialContext = &aStorage.mMaterialContexts.back();

    Handle<const graphics::BufferAny> glyphInstanceBuffer =
        getBufferView(*consolidatedStream, semantic::gGlyphIdx).mGLBuffer;
    Handle<const graphics::BufferAny> instanceToStringEntityBuffer =
        getBufferView(*consolidatedStream, semantic::gEntityIdx).mGLBuffer;

    TextPart result{
        Part{
            .mName = "glyph",
            .mMaterial = Material{
                .mContext = materialContext,
                .mEffect = aEffect,
            },
            .mVertexStream = std::move(consolidatedStream),
            .mPrimitiveMode = GL_TRIANGLE_STRIP,
            .mVertexCount = 4,
        },
    };
    result.mGlyphInstanceBuffer = glyphInstanceBuffer;
    result.mInstanceToStringEntityBuffer = instanceToStringEntityBuffer;
    result.mStringEntitiesUbo = entitiesUbo;
    return result;
}


} // namespace ad::renderer