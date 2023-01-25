#include "Text.h"

#include "../Cube.h"

// TODO remove once not hardcoding a rotation anymore
#include <math/Angle.h>
#include <math/Transformations.h>


namespace ad {
namespace snac {


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


GlyphAtlas::GlyphAtlas(arte::FontFace aFontFace) :
    mFontFace{std::move(aFontFace)},
    mGlyphCache{mFontFace, 20, 126}
{}


std::vector<GlyphInstance> GlyphAtlas::populateInstances(const std::string & aString,
                                                         math::sdr::Rgba aColor) const
{
    math::Size<2, GLfloat> stringDimension_p = graphics::detail::getStringDimension(
        aString,
        mGlyphCache,
        mFontFace);

    std::vector<GlyphInstance> glyphInstances;
    graphics::detail::forEachGlyph(
        aString,
        {0.f, 0.f},
        mGlyphCache,
        mFontFace,
        [&glyphInstances, stringDimension_p, aColor/*, stringPos*/]
        (const graphics::detail::RenderedGlyph & aGlyph, math::Position<2, GLfloat> aGlyphPosition_p)
        {
             glyphInstances.push_back(GlyphInstance{
                .glyphToScreen_p =
                    math::trans2d::translate(aGlyphPosition_p.as<math::Vec>()
                                             - stringDimension_p.as<math::Vec>() / 2.f)
                    * math::trans2d::rotate(math::Degree{45.f})
                    //* math::trans2d::translate(stringPos.as<math::Vec>())
                    ,
                .albedo = aColor,
                .offsetInTexture_p = aGlyph.offsetInTexture,
                .boundingBox_p = aGlyph.controlBoxSize,
                .bearing_p = aGlyph.bearing,
             });
        }
    );

    return glyphInstances;
}


graphics::Program makeDefaultTextProgram(const resource::ResourceFinder & aResourceFinder)
{
    return graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Text.vert"))},
                {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Text.frag"))},
           });
}


Mesh makeGlyphMesh(GlyphAtlas & aGlyphAtlas, graphics::Program aProgram)
{
    auto effect = std::make_shared<Effect>(Effect{
        .mProgram = {
            std::move(aProgram),
            "TextRendering"
        }
    });
    
    auto material = std::make_shared<Material>(Material{
        .mTextures = {{Semantic::FontAtlas, &aGlyphAtlas.mGlyphCache.atlas}},
        .mEffect = std::move(effect)
    });

    return Mesh{
        .mStream = makeRectangle({ {0.f, -1.f}, {1.f, 1.f}}), // The quad goes from [0, -1] to [1, 0]
        .mMaterial = std::move(material),
    };
}


} // namespace snac
} // namespace ad
