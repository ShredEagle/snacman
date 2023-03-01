#include "Renderer.h"

#include "renderer/ScopeGuards.h"

#include <math/Angle.h>
#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/ResourceLoad.h>
#include <snac-renderer/text/Text.h>
#include <snacman/Profiling.h>
#include <snacman/ProfilingGPU.h>
#include <snacman/Resources.h>

#include <platform/Filesystem.h>

// TODO #generic-render remove once all geometry and shader programs are created outside.
#include <snac-renderer/Cube.h>
#include <snac-renderer/gltf/GltfLoad.h>
#include <renderer/ShaderSource.h>


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
    TIME_RECURRING_CLASSFUNC(Render);

    // Note: this is pessimised code.
    // Most of these expensive operations should be taken out and the results
    // cached.
    for (const visu::TextScreen & text : aState.mTextEntities)
    {
        // TODO should be cached once in the string
        math::Size<2, GLfloat> stringDimension_p =
            graphics::detail::getStringDimension(
                text.mString, text.mFont->mGlyphAtlas.mGlyphCache,
                text.mFont->mGlyphAtlas.mFontFace);

        // TODO should be done outside of here (so static strings are not
        // recomputed each frame, for example)
        auto stringPos = text.mPosition_unitscreen.cwMul(
            static_cast<math::Position<2, GLfloat>>(
                aRenderer.mAppInterface.getFramebufferSize()));

        auto scale = math::trans2d::scale(text.mScale);
        auto localToScreen_pixel =
            scale
            * math::trans2d::translate(-stringDimension_p.as<math::Vec>() / 2.f)
            * math::trans2d::rotate(text.mOrientation)
            * math::trans2d::translate(stringDimension_p.as<math::Vec>() / 2.f)
            * math::trans2d::translate(stringPos.as<math::Vec>());

        // TODO should be cached once in the string and forwarded here
        std::vector<snac::GlyphInstance> textBufferData =
            text.mFont->mGlyphAtlas.populateInstances(
                text.mString, to_sdr(text.mColor), localToScreen_pixel, scale);

        // TODO should be consolidated, a single call for all string of the same
        // font.
        mGlyphInstances.respecifyData(std::span{textBufferData});
        BEGIN_RECURRING_GL("Draw string", drawStringProfile);
        auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, false);
        aRenderer.mRenderer.render(text.mFont->mGlyphMesh, mGlyphInstances, aUniforms, aUniformBlocks);
        END_RECURRING_GL(drawStringProfile);
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
        instances.mAttributes.emplace(snac::Semantic::LocalToWorld,
                                      transformation);
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

Renderer::Renderer(graphics::AppInterface & aAppInterface) :
    mAppInterface{aAppInterface},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()),
            snac::Camera::gDefaults},
    mMeshInstances{initializeInstanceStream()}
{}

void Renderer::resetProjection(float aAspectRatio,
                               snac::Camera::Parameters aParameters)
{
    mCamera.resetProjection(aAspectRatio, aParameters);
}


std::shared_ptr<snac::Mesh> Renderer::LoadShape(filesystem::path aShape, snac::Resources & aResources)
{
    if(aShape.string() == "CUBE")
    {
        return std::make_shared<snac::Mesh>(
            snac::loadCube(aResources.getShaderEffect("shaders/PhongLighting.prog")));
    }
    else
    {
        return std::make_shared<snac::Mesh>(
            loadModel(aShape, aResources.getShaderEffect("shaders/PhongLightingTextures.prog")));
    }
}


std::shared_ptr<snac::Font> Renderer::loadFont(arte::FontFace aFontFace,
                                               unsigned int aPixelHeight,
                                               snac::Resources & aResources)
{
    return std::make_shared<snac::Font>(
        std::move(aFontFace),
        aPixelHeight,
        aResources.getShaderEffect("shaders/Text.prog")
    );
}

void Renderer::render(const visu::GraphicState & aState)
{
    TIME_RECURRING(Render, "Render");

    // Stream the instance buffer data
    std::map<snac::Mesh *, std::vector<PoseColor>> sortedMeshes;

    BEGIN_RECURRING(Render, "Sort_meshes", sortMeshProfile);
    for (const visu::Entity & entity : aState.mEntities)
    {
        sortedMeshes[entity.mMesh.get()].push_back(PoseColor{
            .pose = math::trans3d::scale(entity.mScaling)
                    * entity.mOrientation.toRotationMatrix()
                    * math::trans3d::translate(
                        entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
        });
    }
    END_RECURRING(sortMeshProfile);

    // Position camera
    mCamera.setWorldToCamera(aState.mCamera.mWorldToCamera);

    math::hdr::Rgb_f lightColor =  to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::Position<3, GLfloat> lightPosition{0.f, 0.f, 0.f};
    math::hdr::Rgb_f ambientColor =  math::hdr::Rgb_f{0.1f, 0.1f, 0.1f};
    
    snac::UniformRepository uniforms{
        {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
        {snac::Semantic::LightPosition, {lightPosition}},
        {snac::Semantic::AmbientColor, {ambientColor}},
        {snac::Semantic::FramebufferResolution,
         mAppInterface.getFramebufferSize()},
    };

    snac::UniformBlocks uniformBlocks{
        {snac::BlockSemantic::Viewing, &mCamera.mViewing},
    };

    BEGIN_RECURRING_GL("Draw_meshes", drawMeshProfile);
    for (const auto & [mesh, instances] : sortedMeshes)
    {
        mMeshInstances.respecifyData(std::span{instances});
        auto depthTest = graphics::scopeFeature(GL_DEPTH_TEST, true);
        mRenderer.render(*mesh, mMeshInstances, uniforms, uniformBlocks);
    }
    END_RECURRING_GL(drawMeshProfile);

    //
    // Text
    //

    // TODO Why it does not start at 20, but at 32 ????!
    // graphics::detail::RenderedGlyph glyph = mGlyphAtlas.mGlyphMap.at(90);

    mTextRenderer.render(*this, aState, uniforms, uniformBlocks);
}

} // namespace snacgame
} // namespace ad
