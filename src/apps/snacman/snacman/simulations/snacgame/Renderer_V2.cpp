#include "Renderer_V2.h"

#include "renderer/ScopeGuards.h"

#include <graphics/AppInterface.h>

#include <imguiui/Widgets.h> 

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <platform/Filesystem.h>

#include <renderer/BufferLoad.h>
#include <renderer/Uniforms.h>

// TODO #RV2 Remove V1 includes
#include <snac-renderer-V1/text/Text.h>
#include <snac-renderer-V1/Instances.h>
#include <snac-renderer-V1/Semantic.h>
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/ResourceLoad.h>

#include <profiler/GlApi.h>

#include <snac-renderer-V2/Pass.h>
#include <snac-renderer-V2/RendererReimplement.h>
#include <snac-renderer-V2/SetupDrawing.h>

#include <snacman/Profiling.h>
#include <snacman/ProfilingGPU.h>
#include <snacman/Resources.h>

// TODO #generic-render remove once all geometry and shader programs are created
// outside.


static constexpr unsigned int gMaxBones = 32;
static constexpr unsigned int gMaxRiggedInstances = 16;


namespace ad {
namespace snacgame {



Renderer_V2::Renderer_V2(graphics::AppInterface & aAppInterface,
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


Handle<renderer::Node> Renderer_V2::loadModel(filesystem::path aModel,
                                                           filesystem::path aEffect, 
                                                           Resources_t & aResources)
{
    return std::make_shared<renderer::Node>(
        loadBinary(aModel, 
                   mRendererToKeep.mStorage,
                   aResources.loadEffect(aEffect, mRendererToKeep.mStorage),
                   mRendererToKeep.mRenderGraph.mInstanceStream));
}

std::shared_ptr<snac::Font> Renderer_V2::loadFont(arte::FontFace aFontFace,
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


void Renderer_V2::continueGui()
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
void Renderer_V2::renderText(const T_range & aTexts, snac::ProgramSetup & aProgramSetup)
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
        BEGIN_RECURRING_GL("Draw string", drawStringProfile);
        mTextRenderer.render(mDynamicStrings, *text.mFont, mRendererToDecomission, aProgramSetup);
        END_RECURRING_GL(drawStringProfile);
    }
}


void Renderer_V2::render(const visu::GraphicState & aState)
{
    TIME_RECURRING_GL("Render");

    // Stream the instance buffer data
    std::map<renderer::Node *, std::vector<snac::PoseColorSkeleton>> sortedModels;

    BEGIN_RECURRING_GL("Sort_meshes", sortModelProfile);
    // running total of joints for iterated entities
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

        sortedModels[entity.mModel.get()].push_back(snac::PoseColorSkeleton{
            .pose = math::trans3d::scale(entity.mScaling)
                    * entity.mOrientation.toRotationMatrix()
                    * math::trans3d::translate(
                        entity.mPosition_world.as<math::Vec>()),
            // TODO #RV2: No need to go to SDR, we can directly stream hdr floats
            .albedo = to_sdr(entity.mColor),
            .matrixPaletteOffset = matrixPaletteOffset,
        });
    }
    END_RECURRING_GL(sortModelProfile);

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

    if (mControl.mRenderModels)
    {
        // TODO #RV2 render
        //static snac::Camera shadowLightViewPoint{1, 
        //    {
        //        .vFov = math::Degree<float>(95.f),
        //        .zNear = -1.f,
        //        .zFar = -50.f,
        //    }};
        //shadowLightViewPoint.setPose(worldToLight);

        //TIME_RECURRING_GL("Draw_meshes");
        //// Poor man's pool
        //static std::list<snac::InstanceStream> instanceStreams;
        //while(instanceStreams.size() < sortedModels.size())
        //{
        //    instanceStreams.push_back(snac::initializeInstanceStream<snac::PoseColorSkeleton>());
        //}

        //auto streamIt = instanceStreams.begin();
        //std::vector<snac::Pass::Visual> visuals;
        //for (const auto & [model, instances] : sortedModels)
        //{
        //    streamIt->respecifyData(std::span{instances});
        //    for (const auto & mesh : model->mParts)
        //    {
        //        visuals.push_back({&mesh, &*streamIt});
        //    }
        //    ++streamIt;
        //}
        //mPipelineShadows.execute(visuals, shadowLightViewPoint, mRendererToDecomission, programSetup);


        static const renderer::StringKey pass = "forward";

        std::function<void(const renderer::Node &)> recurseNodes;
        recurseNodes = [&, this/*, &recurseNodes*/](const renderer::Node & aNode)
        {
            using renderer::gl;

            const renderer::Instance & instance = aNode.mInstance;

            if(renderer::Object * object = instance.mObject)
            {
                for(const renderer::Part & part : object->mParts)
                {
                    if(renderer::Handle<renderer::ConfiguredProgram> configuredProgram = 
                            renderer::getProgramForPass(*part.mMaterial.mEffect, pass))
                    {
                        // Data is uploaded above, in the previous code
                        //proto::loadSingle(mCameraBuffer.mViewing, , graphics::BufferHint::DynamicDraw);
                        renderer::RepositoryUbo repositoryUbo{
                            {semantic::gViewProjection, &mCameraBuffer.mViewing},
                        };
                        renderer::setBufferBackedBlocks(configuredProgram->mProgram, repositoryUbo);
            
                        // TODO #RV2 make a block for lights
                        graphics::setUniform(configuredProgram->mProgram.mProgram, "u_LightColor", lightColor);
                        graphics::setUniform(configuredProgram->mProgram.mProgram, "u_LightPosition", lightPosition_cam);
                        graphics::setUniform(configuredProgram->mProgram.mProgram, "u_AmbientColor", ambientColor);

                        renderer::Handle<graphics::VertexArrayObject> vao =
                            renderer::getVao(*configuredProgram, part, mRendererToKeep.mStorage);

                        graphics::ScopedBind vaoScope{*vao};
                        graphics::ScopedBind programScope{configuredProgram->mProgram.mProgram};

                        // TODO to be handled by Render Graph passes
                        glEnable(GL_CULL_FACE);
                        glEnable(GL_DEPTH_TEST);
                        glDepthMask(GL_TRUE);

                        gl.DrawElementsInstancedBaseVertex(
                            part.mPrimitiveMode,
                            part.mIndicesCount,
                            part.mVertexStream->mIndicesType,
                            (void*)((size_t)part.mIndexFirst * graphics::getByteSize(part.mVertexStream->mIndicesType)),
                            1, // instance count
                            part.mVertexFirst
                        );
                    }
                }
            }

            for(const auto & child : aNode.mChildren)
            {
                recurseNodes(child);
            }
        };

        TIME_RECURRING_GL("Draw_meshes");
        for (const auto & [model, instances] : sortedModels)
        {
            for (const auto & instance : instances)
            {
                SnacGraph::InstanceData instanceData{
                    instance.pose,
                    instance.albedo,
                };
                renderer::proto::loadSingle(*mRendererToKeep.mRenderGraph.getBufferView(semantic::gLocalToWorld).mGLBuffer,
                                            instanceData,          
                                            // TODO change to DynamicDraw when properly handling AZDO
                                            graphics::BufferHint::StreamDraw);

                recurseNodes(*model);
            }
        }
    }

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
        mDebugRenderer.render(aState.mDebugDrawList, mRendererToDecomission, programSetup);
    }
}

} // namespace snacgame
} // namespace ad