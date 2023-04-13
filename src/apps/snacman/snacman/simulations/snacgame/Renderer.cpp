#include "Renderer.h"

#include "renderer/ScopeGuards.h"

#include <graphics/AppInterface.h>

#include <imguiui/Widgets.h> 

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <platform/Filesystem.h>

#include <snac-renderer/text/Text.h>
#include <snac-renderer/Instances.h>
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



Renderer::Renderer(graphics::AppInterface & aAppInterface,
                   snac::Load<snac::Technique> & aTechniqueAccess,
                   arte::FontFace aDebugFontFace) :
    mAppInterface{aAppInterface},
    mPipelineShadows{aAppInterface, aTechniqueAccess},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()),
            snac::Camera::gDefaults},
    mDebugRenderer{aTechniqueAccess, std::move(aDebugFontFace)}
{
    mPipelineShadows.getControls().mShadowBias = 0.0005f;
}

//void Renderer::resetProjection(float aAspectRatio,
//                               snac::Camera::Parameters aParameters)
//{
//    mCamera.resetProjection(aAspectRatio, aParameters);
//}

std::shared_ptr<snac::Model> Renderer::LoadModel(filesystem::path aModel,
                                                 snac::Resources & aResources)
{
    if (aModel.string() == "CUBE")
    {
        auto model = std::make_shared<snac::Model>();
        model->mParts.push_back({snac::loadCube(aResources.getShaderEffect("effects/Mesh.sefx"))});
        return model;
    }
    else
    {
        return std::make_shared<snac::Model>(
            loadModel(aModel, aResources.getShaderEffect("effects/MeshTextures.sefx")));
    }
}

std::shared_ptr<snac::Font> Renderer::loadFont(arte::FontFace aFontFace,
                                               unsigned int aPixelHeight,
                                               filesystem::path aEffect,
                                               snac::Resources & aResources)
{
    return std::make_shared<snac::Font>(
        std::move(aFontFace),
        aPixelHeight,
        aResources.getShaderEffect(aEffect)
    );
}


void Renderer::continueGui()
{
    using namespace imguiui;

    addCheckbox("Render models", mControl.mRenderModels);
    addCheckbox("Render text", mControl.mRenderText);
    addCheckbox("Render debug", mControl.mRenderDebug);

    ImGui::Checkbox("Show shadow controls", &mControl.mShowShadowControls);
    if (mControl.mShowShadowControls)
    {
        mPipelineShadows.drawGui();
    }
}


void Renderer::renderText(const visu::GraphicState & aState, snac::ProgramSetup & aProgramSetup)
{
    TIME_RECURRING_CLASSFUNC_GL();

    // Note: this is pessimised code.
    // Most of these expensive operations should be taken out and the results
    // cached.
    for (const visu::TextScreen & text : aState.mTextEntities)
    {
        // TODO should be cached once in the string
        math::Size<2, GLfloat> stringDimension_pix =
            graphics::getStringDimension(
                text.mString,
                text.mFont->mFontData.mGlyphMap,
                text.mFont->mFontData.mFontFace);

        auto stringPos_screenPix = text.mPosition_unitscreen.cwMul(
            static_cast<math::Position<2, GLfloat>>(mAppInterface.getFramebufferSize()));

        auto scale = math::trans3d::scale(math::Size<3, GLfloat>{text.mScale, 1.f});
        //auto localPixToScreenPix =
        //    scale
        //    * math::trans2d::translate(-stringDimension_pix.as<math::Vec>() / 2.f)
        //    * math::trans2d::rotate(text.mOrientation)
        //    * math::trans2d::translate(stringDimension_pix.as<math::Vec>() / 2.f)
        //    * math::trans2d::translate(stringPos_screenPix.as<math::Vec>());
        auto localPixToScreenPix = scale
            * math::trans3d::translate(math::Vec<3, GLfloat>{stringPos_screenPix.as<math::Vec>(), 0.f});

        // TODO should be cached once in the string and forwarded here
        std::vector<snac::GlyphInstance> textBufferData =
            text.mFont->mFontData.populateInstances(text.mString,
                                                    to_sdr(text.mColor),
                                                    localPixToScreenPix);

        // TODO should be consolidated, a single call for all string of the same
        // font.
        mDynamicStrings.respecifyData(std::span{textBufferData});
        BEGIN_RECURRING_GL("Draw string", drawStringProfile);
        mTextRenderer.render(mDynamicStrings, *text.mFont, mRenderer, aProgramSetup);
        END_RECURRING_GL(drawStringProfile);
    }
}


void Renderer::render(const visu::GraphicState & aState)
{
    TIME_RECURRING_GL("Render");

    // Stream the instance buffer data
    std::map<snac::Model *, std::vector<snac::PoseColor>> sortedModels;

    BEGIN_RECURRING_GL("Sort_meshes", sortModelProfile);
    for (const visu::Entity & entity : aState.mEntities)
    {
        sortedModels[entity.mModel.get()].push_back(snac::PoseColor{
            .pose = math::trans3d::scale(entity.mScaling)
                    * entity.mOrientation.toRotationMatrix()
                    * math::trans3d::translate(
                        entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
        });
    }
    END_RECURRING_GL(sortModelProfile);

    // Position camera
    mCamera.setWorldToCamera(aState.mCamera.mWorldToCamera);
    // TODO #camera remove that local camera
    snac::Camera cam{
        math::getRatio<float>(mAppInterface.getWindowSize()),
        snac::Camera::gDefaults};
    cam.setPose(aState.mCamera.mWorldToCamera);


    const math::AffineMatrix<4, GLfloat> worldToLight = 
        math::trans3d::rotateX(math::Degree<float>{65.f}) // this is about the worst angle for shadows, on closest labyrinth row
        * math::trans3d::translate<GLfloat>({0.f, 0.f, -14.f});

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
            {snac::Semantic::FramebufferResolution, mAppInterface.getFramebufferSize()},
            {snac::Semantic::ViewingMatrix, cam.assembleViewMatrix()}
        },
        .mUniformBlocks{
            {snac::BlockSemantic::Viewing, &mCamera.mViewing},
        }
    };

    if (mControl.mRenderModels)
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
        while(instanceStreams.size() < sortedModels.size())
        {
            instanceStreams.push_back(snac::initializeInstanceStream<snac::PoseColor>());
        }

        auto streamIt = instanceStreams.begin();
        std::vector<snac::Pass::Visual> visuals;
        for (const auto & [model, instances] : sortedModels)
        {
            streamIt->respecifyData(std::span{instances});
            for (const auto & mesh : model->mParts)
            {
                visuals.push_back({&mesh, &*streamIt});
            }
            ++streamIt;
        }
        mPipelineShadows.execute(visuals, shadowLightViewPoint, mRenderer, programSetup);
    }

    //
    // Text
    //
    {
        auto scope = programSetup.mUniforms.push(snac::Semantic::ViewingMatrix,
                                                 math::Matrix<4, 4, GLfloat>::Identity());
        if (mControl.mRenderText)
        {
            renderText(aState, programSetup);
        }
    }

    if (mControl.mRenderDebug)
    {
        TIME_RECURRING_GL("Draw_debug");
        mDebugRenderer.render(aState.mDebugDrawList, mRenderer, programSetup);
    }
}

} // namespace snacgame
} // namespace ad
