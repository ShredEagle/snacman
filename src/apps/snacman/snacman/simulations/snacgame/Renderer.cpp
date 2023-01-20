#include "Renderer.h"

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
        math::Position<2, GLfloat> position;
        math::sdr::Rgba albedo;
        GLint offsetInTexture_p; // horizontal offset to the glyph in its ribbon texture.
        math::Size<2, GLfloat> boundingBox_p; // glyph bounding box in texture pixel coordinates.
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
        graphics::ClientAttribute instancePosition{
            .mDimension = 2,
            .mOffset = offsetof(GlyphInstance, position),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::InstancePosition, instancePosition);
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
    instances.mInstanceBuffer.mStride = sizeof(GlyphInstance);
    return instances;
}

snac::Mesh makeGlyphMesh(const arte::Freetype & aFreetype,
                         Renderer::GlyphAtlas & aGlyphAtlas,
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
    
    arte::FontFace face = aFreetype.load(*aResourceFinder.find("fonts/Comfortaa-Regular.ttf"));
    face.inverseYAxis(true)
        .setPixelHeight(100);

    aGlyphAtlas.mGlyphTexture = std::make_shared<graphics::Texture>(
        graphics::detail::makeTightGlyphAtlas(face,
                                              20, 126,
                                              aGlyphAtlas.mGlyphMap)
    );

    auto material = std::make_shared<snac::Material>(snac::Material{
        .mTextures = {{snac::Semantic::FontAtlas, aGlyphAtlas.mGlyphTexture.get()}},
        .mEffect = std::move(effect)
    });

    return snac::Mesh{
        .mStream = snac::makeQuad(),
        .mMaterial = std::move(material),
    };
}

Renderer::Renderer(graphics::AppInterface & aAppInterface, snac::Camera::Parameters aCameraParameters, const resource::ResourceFinder & aResourceFinder) :
    mAppInterface{aAppInterface},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()), aCameraParameters},
    mCubeMesh{.mStream = snac::makeCube()},
    mCubeInstances{initializeInstanceStream()},
    mGlyphMesh{makeGlyphMesh(mFreetype, mGlyphAtlas, aResourceFinder)},
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
    graphics::detail::RenderedGlyph glyph = mGlyphAtlas.mGlyphMap.at(90);
    std::vector<GlyphInstance> textBufferData{{
        GlyphInstance{
            .position = {0.1f, -0.2f},
            .albedo = math::sdr::gGreen,
            .offsetInTexture_p = glyph.offsetInTexture,
            .boundingBox_p = glyph.controlBoxSize,
        },
    }};
    mGlyphInstances.respecifyData(std::span{textBufferData});
    mRenderer.render(mGlyphMesh, mGlyphInstances, uniforms, uniformBlocks);
}


} // namespace cubes
} // namespace ad
