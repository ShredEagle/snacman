#include "Renderer.h"

#include "renderer/ScopeGuards.h"

#include <graphics/AppInterface.h>
#include <math/Transformations.h>
#include <math/VectorUtilities.h>
#include <platform/Filesystem.h>

#include <snac-renderer/text/Text.h>
#include <snac-renderer/Semantic.h>
#include <snac-renderer/Render.h>
#include <snac-renderer/Mesh.h>
#include <snac-renderer/ResourceLoad.h>

#include <snacman/Profiling.h>
#include <snacman/ProfilingGPU.h>
#include <snacman/Resources.h>

// TODO #generic-render remove once all geometry and shader programs are created
// outside.

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
                          snac::ProgramSetup & aProgramSetup)
{
    TIME_RECURRING_CLASSFUNC(Render);

    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, false);

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
        aRenderer.mTextPass.draw(text.mFont->mGlyphMesh,
                                 mGlyphInstances,
                                 aRenderer.mRenderer,
                                 aProgramSetup);
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

Renderer::Renderer(graphics::AppInterface & aAppInterface, snac::Load<snac::Technique> & aTechniqueAccess) :
    mAppInterface{aAppInterface},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()),
            snac::Camera::gDefaults},
    mPipelineShadows{aAppInterface, aTechniqueAccess},
    mMeshInstances{initializeInstanceStream()}
{}

void Renderer::resetProjection(float aAspectRatio,
                               snac::Camera::Parameters aParameters)
{
    mCamera.resetProjection(aAspectRatio, aParameters);
}

std::shared_ptr<snac::Mesh> Renderer::LoadShape(filesystem::path aShape,
                                                snac::Resources & aResources)
{
    if (aShape.string() == "CUBE")
    {
        return std::make_shared<snac::Mesh>(
            snac::loadCube(aResources.getShaderEffect("effects/Mesh.sefx")));
    }
    else
    {
        return std::make_shared<snac::Mesh>(
            loadModel(aShape, aResources.getShaderEffect("effects/MeshTextures.sefx")));
    }
}

std::shared_ptr<snac::Font> Renderer::loadFont(arte::FontFace aFontFace,
                                               unsigned int aPixelHeight,
                                               snac::Resources & aResources)
{
    return std::make_shared<snac::Font>(
        std::move(aFontFace),
        aPixelHeight,
        aResources.getShaderEffect("effects/Text.sefx")
    );
}


void Renderer::continueGui()
{
    // This boolean is only accessed by main thread
    static bool showShadowControls = false;
    ImGui::Checkbox("Shadows", &showShadowControls);
    showShadowControls = showShadowControls;
    if (showShadowControls)
    {
        mPipelineShadows.drawGui();
    }
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

    const math::AffineMatrix<4, GLfloat> worldToLight = 
        math::trans3d::rotateX(math::Degree<float>{65.f}) // this is about the worst angle for shadows, on closest labyrinth row
        * math::trans3d::translate<GLfloat>({-8.f, -6.f, -10.f});

    math::Position<3, GLfloat> lightPosition_cam = 
        (math::homogeneous::makePosition(math::Position<3, GLfloat>::Zero()) // light position in light space is the origin
        * worldToLight.inverse()
        * aState.mCamera.mWorldToCamera).xyz();

    math::hdr::Rgb_f lightColor = to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::hdr::Rgb_f ambientColor = math::hdr::Rgb_f{0.1f, 0.1f, 0.1f};


    snac::ProgramSetup programSetup{
        .mUniforms{
            {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
            {snac::Semantic::LightPosition, {lightPosition_cam}},
            {snac::Semantic::AmbientColor, {ambientColor}},
            {snac::Semantic::FramebufferResolution,
            mAppInterface.getFramebufferSize()},
        },
        .mUniformBlocks{
            {snac::BlockSemantic::Viewing, &mCamera.mViewing},
        }
    };

    {
        static snac::Camera shadowLightViewPoint{1, 
            {
                .vFov = math::Degree<float>(75.f),
                .zNear = -1.f,
                .zFar = -50.f,
            }};
        shadowLightViewPoint.setPose(worldToLight);

            TIME_RECURRING_GL("Draw_meshes");
        // Poor man's pool
        static std::list<snac::InstanceStream> instanceStreams;
        while(instanceStreams.size() < sortedMeshes.size())
        {
            instanceStreams.push_back(initializeInstanceStream());
        }

        auto streamIt = instanceStreams.begin();
        std::vector<snac::Pass::Visual> visuals;
        for (const auto & [mesh, instances] : sortedMeshes)
        {
            streamIt->respecifyData(std::span{instances});
            visuals.push_back({mesh, &*streamIt});
            ++streamIt;
        }
        mPipelineShadows.execute(visuals, shadowLightViewPoint, mRenderer, programSetup);
    }

    //
    // Text
    //
    mTextRenderer.render(*this, aState, programSetup);
}

} // namespace snacgame
} // namespace ad
