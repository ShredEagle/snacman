#include "Renderer_V1.h"

#include "renderer/ScopeGuards.h"

#include <graphics/AppInterface.h>

#include <imguiui/Widgets.h> 

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <platform/Filesystem.h>

#include <renderer/BufferLoad.h>

#include <snac-renderer-V1/text/Text.h>
#include <snac-renderer-V1/Instances.h>
#include <snac-renderer-V1/Semantic.h>
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/ResourceLoad.h>

#include <snacman/Profiling.h>
#include <snacman/ProfilingGPU.h>
#include <snacman/Resources.h>

// TODO #generic-render remove once all geometry and shader programs are created
// outside.


static constexpr unsigned int gMaxBones = 32;
static constexpr unsigned int gMaxRiggedInstances = 16;


namespace ad {
namespace snacgame {



Renderer::Renderer(graphics::AppInterface & aAppInterface,
                   snac::Load<snac::Technique> & aTechniqueAccess,
                   arte::FontFace aDebugFontFace) :
    mAppInterface{aAppInterface},
    mPipelineShadows{aAppInterface, aTechniqueAccess},
    mCamera{math::getRatio<float>(mAppInterface.getWindowSize()), snac::Camera::gDefaults},
    mDebugRenderer{aTechniqueAccess, std::move(aDebugFontFace)}
{
    graphics::initialize<math::AffineMatrix<4, float>>(mJointMatrices,
                                                       gMaxBones * gMaxRiggedInstances,
                                                       graphics::BufferHint::DynamicDraw);
    mPipelineShadows.getControls().mShadowBias = 0.0005f;
}

//void Renderer::resetProjection(float aAspectRatio,
//                               snac::Camera::Parameters aParameters)
//{
//    mCamera.resetProjection(aAspectRatio, aParameters);
//}

std::shared_ptr<snac::Model> Renderer::LoadModel(filesystem::path aModel,
                                                 filesystem::path aEffect, 
                                                 snac::Resources & aResources)
{
    if (aModel.string() == "CUBE")
    {
        auto model = std::make_shared<snac::Model>();
        model->mParts.push_back({snac::loadCube(aResources.getShaderEffect(aEffect))});
        return model;
    }
    else
    {
        return std::make_shared<snac::Model>(
            loadModel(aModel, aResources.getShaderEffect(aEffect)));
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


template <class T_range>
void Renderer::renderText(const T_range & aTexts, snac::ProgramSetup & aProgramSetup)
{
    // Note: this is pessimised code.
    // Most of these expensive operations should be taken out and the results
    // cached.
    for (const visu::Text & text : aTexts)
    {
        auto localToWorld = 
            math::trans3d::scale(text.mScaling)
            * text.mOrientation.toRotationMatrix()
            * math::trans3d::translate(text.mPosition_world.as<math::Vec>());

        // TODO should be cached once in the string and forwarded here
        std::vector<snac::GlyphInstance> textBufferData =
            text.mFont->mFontData.populateInstances(text.mString,
                                                    to_sdr(text.mColor),
                                                    localToWorld);

        // TODO should be consolidated, a single call for all string of the same
        // font.
        mDynamicStrings.respecifyData(std::span{textBufferData});
        auto drawStringEntry = BEGIN_RECURRING_GL("Draw string", drawStringProfile);
        mTextRenderer.render(mDynamicStrings, *text.mFont, mRenderer, aProgramSetup);
        END_RECURRING_GL(drawStringProfile, drawStringEntry);
    }
}


void Renderer::render(const visu::GraphicState & aState)
{
    TIME_RECURRING_GL("Render");

    // Stream the instance buffer data
    std::map<renderer::Handle<const renderer::Object>,
             std::vector<snac::PoseColorSkeleton>> sortedModels;

    auto sortModelEntry = BEGIN_RECURRING_GL("Sort_meshes", sortModelProfile);
    GLuint jointMatricesCount = 0;
    for (const visu::Entity & entity : aState.mEntities)
    {
        GLuint matrixPaletteOffset = std::numeric_limits<GLuint>::max();
        if(entity.mRigging.mAnimation != nullptr)
        {
            {
                TIME_RECURRING_GL("Prepare_joint_matrices");

                entity.mRigging.mAnimation->animate(
                    (float)entity.mRigging.mParameterValue,
                    entity.mRigging.mRig->mScene);

                const auto jointMatrices = entity.mRigging.mRig->computeJointMatrices();
                graphics::replaceSubset(mJointMatrices, jointMatricesCount, std::span{jointMatrices});
            }

            matrixPaletteOffset = jointMatricesCount;
            jointMatricesCount += (GLuint)entity.mRigging.mRig->mJoints.size();
        }

        sortedModels[entity.mModel].push_back(snac::PoseColorSkeleton{
            .pose = math::trans3d::scale(entity.mScaling)
                    * entity.mOrientation.toRotationMatrix()
                    * math::trans3d::translate(
                        entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
            .matrixPaletteOffset = matrixPaletteOffset,
        });
    }
    END_RECURRING_GL(sortModelProfile, sortModelEntry);

    // Position camera
    // TODO #camera The Camera instance should come from the graphic state directly
    mCamera.setPose(aState.mCamera.mWorldToCamera);
    mCameraBuffer.set(mCamera);


    static const math::AffineMatrix<4, GLfloat> worldToLight = 
        math::trans3d::rotateX(math::Degree<float>{60.f})
        * math::trans3d::translate<GLfloat>({0.f, 2.5f, -16.f});

    math::Position<3, GLfloat> lightPosition_cam = 
        (math::homogeneous::makePosition(math::Position<3, GLfloat>::Zero()) // light position in light space is the origin
        * worldToLight.inverse()
        * aState.mCamera.mWorldToCamera).xyz();

    math::hdr::Rgb_f lightColor = to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::hdr::Rgb_f ambientColor = math::hdr::Rgb_f{0.4f, 0.4f, 0.4f};

    const math::Size<2, int> framebufferSize = mAppInterface.getFramebufferSize();

    snac::ProgramSetup programSetup{
        .mUniforms{
            {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
            {snac::Semantic::LightPosition, {lightPosition_cam}},
            {snac::Semantic::AmbientColor, {ambientColor}},
            {snac::Semantic::FramebufferResolution, framebufferSize},
            {snac::Semantic::ViewingMatrix, mCamera.assembleViewMatrix()}
        },
        .mUniformBlocks{
            {snac::BlockSemantic::Viewing, &mCameraBuffer.mViewing},
            {snac::BlockSemantic::JointMatrices, &mJointMatrices},
        }
    };

    // TODO #RV2 Remove this segment, when we have a V2 Render Graph
    // In the process of being decommissioned
    //if (mControl.mRenderModels)
    //{
    //    static snac::Camera shadowLightViewPoint{1, 
    //        {
    //            .vFov = math::Degree<float>(95.f),
    //            .zNear = -1.f,
    //            .zFar = -50.f,
    //        }};
    //    shadowLightViewPoint.setPose(worldToLight);

    //    TIME_RECURRING_GL("Draw_meshes");
    //    // Poor man's pool
    //    static std::list<snac::InstanceStream> instanceStreams;
    //    while(instanceStreams.size() < sortedModels.size())
    //    {
    //        instanceStreams.push_back(snac::initializeInstanceStream<snac::PoseColorSkeleton>());
    //    }

    //    auto streamIt = instanceStreams.begin();
    //    std::vector<snac::Pass::Visual> visuals;
    //    for (const auto & [model, instances] : sortedModels)
    //    {
    //        streamIt->respecifyData(std::span{instances});
    //        for (const auto & mesh : model->mParts)
    //        {
    //            visuals.push_back({&mesh, &*streamIt});
    //        }
    //        ++streamIt;
    //    }
    //    mPipelineShadows.execute(visuals, shadowLightViewPoint, mRenderer, programSetup);
    //}

    //
    // Text
    //
    {
        TIME_RECURRING_GL("Draw_texts");

        // 3D world text
        if (mControl.mRenderText)
        {
            // This text should be occluded by geometry in front of it.
            // WARNING: (smelly) the text rendering disable depth writes,
            //          so it must be drawn last to occlude other visuals in the 3D world.
            auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);
            renderText(aState.mTextWorldEntities, programSetup);
        }

        // For the screen space text, the viewing transform is composed as follows:
        // The world-to-camera is identity
        // The projection is orthographic, mapping framebuffer resolution (with origin at screen center) to NDC.
        auto scope = programSetup.mUniforms.push(
            snac::Semantic::ViewingMatrix,
            math::trans3d::orthographicProjection(
                math::Box<float>{
                    {-static_cast<math::Position<2, float>>(framebufferSize) / 2, -1.f},
                    {static_cast<math::Size<2, float>>(framebufferSize), 2.f}
                })
        );

        if (mControl.mRenderText)
        {
            renderText(aState.mTextScreenEntities, programSetup);
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
