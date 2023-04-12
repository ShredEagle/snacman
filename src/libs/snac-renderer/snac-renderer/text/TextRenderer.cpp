#include "TextRenderer.h"


namespace ad {
namespace snac {


namespace {

    InstanceStream initializeGlyphInstanceStream()
    {
        InstanceStream instances;
        {
            graphics::ClientAttribute localToWorld{
                .mDimension = {3, 3},
                .mOffset = offsetof(GlyphInstance, glyphToScreen_p),
                .mComponentType = GL_FLOAT,
            };
            instances.mAttributes.emplace(Semantic::LocalToWorld, localToWorld);
        }
        {
            graphics::ClientAttribute albedo{
                .mDimension = 4,
                .mOffset = offsetof(GlyphInstance, albedo),
                .mComponentType = GL_UNSIGNED_BYTE,
            };
            instances.mAttributes.emplace(Semantic::Albedo, albedo);
        }
        {
            graphics::ClientAttribute entryIndex{
                .mDimension = 1,
                .mOffset = offsetof(GlyphInstance, entryIndex),
                .mComponentType = GL_UNSIGNED_INT,
            };
            instances.mAttributes.emplace(Semantic::GlyphIndex, entryIndex);
        }
        instances.mInstanceBuffer.mStride = sizeof(GlyphInstance);
        return instances;
    }

} // anonymous namespace


GlyphInstanceStream::GlyphInstanceStream() :
    InstanceStream{initializeGlyphInstanceStream()}
{}


void TextRenderer::render(const GlyphInstanceStream & aGlyphs,
                          const Font & aFont,
                          Renderer & aRenderer,
                          snac::ProgramSetup & aProgramSetup)
{
    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, false);

    mTextPass.draw(aFont.mGlyphMesh,
                   aGlyphs,
                   aRenderer,
                   aProgramSetup);
}


} // namespace snac
} // namespace ad
