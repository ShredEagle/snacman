#include "TextRenderer.h"


namespace ad {
namespace snac {


namespace {

    InstanceStream initializeGlyphInstanceStream()
    {
        InstanceStream instances;
        {
            graphics::ClientAttribute instancePosition{
                .mDimension = 2,
                .mOffset = offsetof(GlyphInstance, glyphToString_p),
                .mComponentType = GL_FLOAT,
            };
            instances.mAttributes.emplace(Semantic::InstancePosition, instancePosition);
        }
        {
            graphics::ClientAttribute localToWorld{
                .mDimension = {4, 4},
                .mOffset = offsetof(GlyphInstance, stringToWorld),
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
    auto depthWriteScope = graphics::scopeDepthMask(false);

    mTextPass.draw(aFont.mGlyphMesh,
                   aGlyphs,
                   aRenderer,
                   aProgramSetup);
}


} // namespace snac
} // namespace ad
