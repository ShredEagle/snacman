#include "Renderer.h"

#include <math/Angle.h>
#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/Cube.h>


namespace ad {
namespace snacgame {


namespace {

    struct PoseColor
    {
        math::Matrix<4, 4, float> pose;
        math::sdr::Rgba albedo;
    };
    
    struct GlyphInstance
    {
        math::AffineMatrix<3, GLfloat> glyphToScreen_p;
        math::sdr::Rgba albedo;
        GLint offsetInTexture_p; // horizontal offset to the glyph in its ribbon texture.
        math::Size<2, GLfloat> boundingBox_p; // glyph bounding box in texture pixel coordinates, including margins.
        math::Vec<2, GLfloat> bearing_p; // bearing, including  margins.
    };

} // anonymous namespace


snac::InstanceStream initializeInstanceStream()
{
    snac::InstanceStream instances;
    {
        graphics::ClientAttribute transformation{
            .mDimension = {4, 4},
            .mOffset = offsetof(PoseColor, pose),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::LocalToWorld, transformation);
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(PoseColor, albedo),
            .mComponentType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(snac::Semantic::Albedo, albedo);
    }
    instances.mInstanceBuffer.mStride = sizeof(PoseColor);
    return instances;
}


snac::InstanceStream initializeGlyphInstanceStream()
{
    snac::InstanceStream instances;
    {
        graphics::ClientAttribute localToWorld{
            .mDimension = {3, 3},
            .mOffset = offsetof(GlyphInstance, glyphToScreen_p),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::LocalToWorld, localToWorld);
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(GlyphInstance, albedo),
            .mComponentType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(snac::Semantic::Albedo, albedo);
    }
    {
        graphics::ClientAttribute textureOffset{
            .mDimension = 1,
            .mOffset = offsetof(GlyphInstance, offsetInTexture_p),
            .mComponentType = GL_INT,
        };
        instances.mAttributes.emplace(snac::Semantic::TextureOffset, textureOffset);
    }
    {
        graphics::ClientAttribute boundingBox{
            .mDimension = 2,
            .mOffset = offsetof(GlyphInstance, boundingBox_p),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::BoundingBox, boundingBox);
    }
    {
        graphics::ClientAttribute bearing{
            .mDimension = 2,
            .mOffset = offsetof(GlyphInstance, bearing_p),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::Bearing, bearing);
    }
    instances.mInstanceBuffer.mStride = sizeof(GlyphInstance);
    return instances;
}

Renderer::GlyphAtlas::GlyphAtlas(arte::FontFace aFontFace) :
    mFontFace{std::move(aFontFace)},
    mGlyphCache{mFontFace, 20, 126}
{}

snac::Mesh makeGlyphMesh(Renderer::GlyphAtlas & aGlyphAtlas,
                         const resource::ResourceFinder & aResourceFinder)
{
    auto effect = std::make_shared<snac::Effect>(snac::Effect{
        .mProgram = {
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Text.vert"))},
                {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Text.frag"))},
            }),
            "TextRendering"
        }
    });
    
    auto material = std::make_shared<snac::Material>(snac::Material{
        .mTextures = {{snac::Semantic::FontAtlas, &aGlyphAtlas.mGlyphCache.atlas}},
        .mEffect = std::move(effect)
    });

    return snac::Mesh{
        .mStream = snac::makeRectangle({ {0.f, -1.f}, {1.f, 1.f}}), // The quad goes from [0, -1] to [1, 0]
        .mMaterial = std::move(material),
    };
}

Renderer::Renderer(graphics::AppInterface & aAppInterface, snac::Camera::Parameters aCameraParameters, const resource::ResourceFinder & aResourceFinder) :
    mAppInterface{aAppInterface},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()), aCameraParameters},
    mCubeMesh{.mStream = snac::makeCube()},
    mCubeInstances{initializeInstanceStream()},
    mFreetype{},
    mGlyphAtlas{std::move(mFreetype.load(*aResourceFinder.find("fonts/Comfortaa-Regular.ttf")).setPixelHeight(100).inverseYAxis(true))},
    mGlyphMesh{makeGlyphMesh(mGlyphAtlas, aResourceFinder)},
    mGlyphInstances{initializeGlyphInstanceStream()}
{
    mCubeMesh.mMaterial = std::make_shared<snac::Material>();
    mCubeMesh.mMaterial->mEffect = std::make_shared<snac::Effect>(snac::Effect{
        .mProgram = {
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.vert"))},
                {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.frag"))},
            }),
            "PhongLighting"
        }
    });
}

void Renderer::resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters)
{
    mCamera.resetProjection(aAspectRatio, aParameters);
}


void Renderer::render(const visu::GraphicState & aState)
{
    // Stream the instance buffer data
    std::vector<PoseColor> instanceBufferData;
    for (const visu::Entity & entity : aState.mEntities)
    {
        instanceBufferData.push_back(PoseColor{
            .pose = 
                math::trans3d::scale(entity.mScaling)
                * math::trans3d::rotateY(entity.mYAngle)
                * math::trans3d::translate(entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
        });
    }

    // Position camera
    mCamera.setWorldToCamera(aState.mCamera.mWorldToCamera);

    math::hdr::Rgb_f lightColor =  to_hdr<float>(math::sdr::gBlue);
    math::Position<3, GLfloat> lightPosition{0.f, 0.f, 0.f};
    math::hdr::Rgb_f ambientColor =  math::hdr::Rgb_f{0.1f, 0.2f, 0.1f};
    
    snac::UniformRepository uniforms{
        {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
        {snac::Semantic::LightPosition, {lightPosition}},
        {snac::Semantic::AmbientColor, {ambientColor}},
        {snac::Semantic::FramebufferResolution, mAppInterface.getFramebufferSize()},
    };

    snac::UniformBlocks uniformBlocks{
         {snac::BlockSemantic::Viewing, &mCamera.mViewing},
    };

    mCubeInstances.respecifyData(std::span{instanceBufferData});
    mRenderer.render(mCubeMesh, mCubeInstances, uniforms, uniformBlocks);

    //
    // Text
    //

    // TODO Why it does not start at 20, but at 32 ????!
    //graphics::detail::RenderedGlyph glyph = mGlyphAtlas.mGlyphMap.at(90);

    math::Size<2, GLfloat> stringDimension_p = graphics::detail::getStringDimension(
        "My string!",
        mGlyphAtlas.mGlyphCache,
        mGlyphAtlas.mFontFace);

    auto stringPos = 
        math::Position<2, GLfloat>{-0.5f, 0.f}
            .cwMul(static_cast<math::Position<2, GLfloat>>(mAppInterface.getFramebufferSize()));

    std::vector<GlyphInstance> textBufferData;
    graphics::detail::forEachGlyph(
        "My string!",
        //"abcdefghijklmnopqrstuvwxyz",
        {0.f, 0.f},
        mGlyphAtlas.mGlyphCache,
        mGlyphAtlas.mFontFace,
        [&textBufferData, stringDimension_p, stringPos]
        (const graphics::detail::RenderedGlyph & aGlyph, math::Position<2, GLfloat> aGlyphPosition_p)
        {
             textBufferData.push_back(GlyphInstance{
                .glyphToScreen_p =
                    math::trans2d::translate(aGlyphPosition_p.as<math::Vec>()
                                             - stringDimension_p.as<math::Vec>() / 2.f)
                    * math::trans2d::rotate(math::Degree{45.f})
                    * math::trans2d::translate(stringPos.as<math::Vec>()),
                .albedo = math::sdr::gGreen,
                .offsetInTexture_p = aGlyph.offsetInTexture,
                .boundingBox_p = aGlyph.controlBoxSize,
                .bearing_p = aGlyph.bearing,
             });
        }
    );
    mGlyphInstances.respecifyData(std::span{textBufferData});
    mRenderer.render(mGlyphMesh, mGlyphInstances, uniforms, uniformBlocks);
}


} // namespace cubes
} // namespace ad
