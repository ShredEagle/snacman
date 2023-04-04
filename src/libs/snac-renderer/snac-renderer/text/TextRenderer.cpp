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
            graphics::ClientAttribute textureOffset{
                .mDimension = 1,
                .mOffset = offsetof(GlyphInstance, offsetInTexture_p),
                .mComponentType = GL_INT,
            };
            instances.mAttributes.emplace(Semantic::TextureOffset, textureOffset);
        }
        {
            graphics::ClientAttribute boundingBox{
                .mDimension = 2,
                .mOffset = offsetof(GlyphInstance, boundingBox_p),
                .mComponentType = GL_FLOAT,
            };
            instances.mAttributes.emplace(Semantic::BoundingBox, boundingBox);
        }
        {
            graphics::ClientAttribute bearing{
                .mDimension = 2,
                .mOffset = offsetof(GlyphInstance, bearing_p),
                .mComponentType = GL_FLOAT,
            };
            instances.mAttributes.emplace(Semantic::Bearing, bearing);
        }
        instances.mInstanceBuffer.mStride = sizeof(GlyphInstance);
        return instances;
    }

} // anonymous namespace


TextRenderer::TextRenderer() :
    mGlyphInstances{initializeGlyphInstanceStream()}
{}


void TextRenderer::respecifyInstanceData(std::span<GlyphInstance> aInstances)
{
    mGlyphInstances.respecifyData(aInstances);
}

void TextRenderer::render(const Font & aFont, Renderer & aRenderer, snac::ProgramSetup & aProgramSetup)
{
    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, false);

    mTextPass.draw(aFont.mGlyphMesh,
                   mGlyphInstances,
                   aRenderer,
                   aProgramSetup);
}


} // namespace snac
} // namespace ad
