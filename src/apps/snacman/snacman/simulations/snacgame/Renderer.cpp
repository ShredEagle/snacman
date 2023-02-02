#include "Renderer.h"

#include <math/Angle.h>
#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/text/Text.h>


namespace ad {
namespace snacgame {


namespace {

    struct PoseColor
    {
        math::Matrix<4, 4, float> pose;
        math::sdr::Rgba albedo;
    };
    
} // anonymous namespace


TextRenderer::TextRenderer() :
    mGlyphInstances{snac::initializeGlyphInstanceStream()}
{}


void TextRenderer::render(Renderer & aRenderer,
                          const visu::GraphicState & aState,
                          const snac::UniformRepository & aUniforms,
                          const snac::UniformBlocks & aUniformBlocks)
{
    // Note: this is pessimised code.
    // Most of these expensive operations should be taken out and the results cached.
    for (const visu::TextScreen & text : aState.mTextEntities)
    {
        // TODO should be cached once in the string
        math::Size<2, GLfloat> stringDimension_p = graphics::detail::getStringDimension(
            text.mString,
            text.mFont->mGlyphAtlas.mGlyphCache,
            text.mFont->mGlyphAtlas.mFontFace);

        // TODO should be done outside of here (so static strings are not recomputed each frame, for example)
        auto stringPos = 
            text.mPosition_unitscreen
                .cwMul(static_cast<math::Position<2, GLfloat>>(aRenderer.mAppInterface.getFramebufferSize()));

        auto localToScreen_pixel = 
            math::trans2d::translate(- stringDimension_p.as<math::Vec>() / 2.f)
            * math::trans2d::rotate(text.mOrientation)
            * math::trans2d::translate(stringDimension_p.as<math::Vec>() / 2.f)
            * math::trans2d::translate(stringPos.as<math::Vec>())
            ;

        // TODO should be cached once in the string and forwarded here
        std::vector<snac::GlyphInstance> textBufferData = 
            text.mFont->mGlyphAtlas.populateInstances(text.mString, to_sdr(text.mColor), localToScreen_pixel);


        // TODO should be consolidated, a single call for all string of the same font.
        mGlyphInstances.respecifyData(std::span{textBufferData});
        aRenderer.mRenderer.render(text.mFont->mGlyphMesh, mGlyphInstances, aUniforms, aUniformBlocks);
    }

}


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


Renderer::Renderer(graphics::AppInterface & aAppInterface, snac::Camera::Parameters aCameraParameters, const resource::ResourceFinder & aResourceFinder) :
    mAppInterface{aAppInterface},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()), aCameraParameters},
    mCubeMesh{std::move(*Renderer::LoadShape(aResourceFinder))},
    mCubeInstances{initializeInstanceStream()}
{}

void Renderer::resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters)
{
    mCamera.resetProjection(aAspectRatio, aParameters);
}


std::shared_ptr<snac::Mesh> Renderer::LoadShape(const resource::ResourceFinder & aResourceFinder)
{
    auto mesh = std::make_shared<snac::Mesh>(snac::Mesh{.mStream = snac::makeCube()}); 

    mesh->mMaterial = std::make_shared<snac::Material>();
    mesh->mMaterial->mEffect = std::make_shared<snac::Effect>(snac::Effect{
        .mProgram = {
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.vert"))},
                {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.frag"))},
            }),
            "PhongLighting"
        },
    });

    return mesh;
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
                * entity.mOrientation.toRotationMatrix()
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

    mTextRenderer.render(*this, aState, uniforms, uniformBlocks);
}


} // namespace cubes
} // namespace ad
