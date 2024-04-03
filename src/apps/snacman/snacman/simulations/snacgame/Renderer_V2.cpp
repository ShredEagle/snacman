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

#include <snacman/Logging.h>
#include <snacman/Profiling.h>
#include <snacman/ProfilingGPU.h>
#include <snacman/Resources.h>
#include <snacman/TemporaryRendererHelpers.h>

#include <file-processor/Processor.h>

#include <cassert>

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
    mPipelineShadows.getControls().mShadowBias = 0.0005f;
}


renderer::Handle<const renderer::Object> Renderer_V2::loadModel(filesystem::path aModel,
                                                                filesystem::path aEffect, 
                                                                Resources_t & aResources)
{
    if(auto loadResult = loadBinary(aModel, 
                                    mRendererToKeep.mStorage,
                                    aResources.loadEffect(aEffect, mRendererToKeep.mStorage),
                                    mRendererToKeep.mRenderGraph.mInstanceStream);
        std::holds_alternative<renderer::Node>(loadResult))
    {
        // TODO Ad 2024/03/27: This does not feel right:
        // we would like the models to be consistent discretes Objects,
        // instead of Nodes with potential trees behind them.
        // Yet our .seum file format can host Node trees (which I think is a good thing).
        // Somehow we have to reconciliate the two, and the current approach seems too brittle.
        renderer::Handle<const renderer::Object> object = recurseToObject(std::get<renderer::Node>(loadResult));
        if(object == renderer::gNullHandle)
        {
            throw std::logic_error{"There should have been an object in the Node hierarchy of a VisualModel"};
        }
        return object;
    }
    else
    {
        auto errorCode = std::get<renderer::SeumErrorCode>(loadResult);
        if(auto basePath = filesystem::path{aModel}.replace_extension(".gltf");
           errorCode == renderer::SeumErrorCode::OutdatedVersion && is_regular_file(basePath))
        {
            SELOG(warn)("Failed to load '{}', error code '{}'. Attempting to re-process file '{}'.", 
                        aModel.string(),
                        (unsigned int)errorCode,
                        basePath.string());
            ad::renderer::processModel(basePath);
            // Note: This recursion intention is to try again **one** time, since the seum file has been processed
            //       and should now match the current version (which would prevent re-satisfying this condition)
            //       This is quite brittle, and could lead to infinite loops.
            return loadModel(aModel, aEffect, aResources);
        }

        SELOG(critical)("Failed to load '{}', error code '{}'.", 
                        aModel.string(),
                        (unsigned int)errorCode);
        throw std::runtime_error{"Invalid binary file in loadModel()."};
    }
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
    for (const visu_V1::Text & text : aTexts)
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
        auto drawStringEntry = BEGIN_RECURRING_GL("Draw string");
        mTextRenderer.render(mDynamicStrings, *text.mFont, mRendererToDecomission, aProgramSetup);
        END_RECURRING_GL(drawStringEntry);
    }
}


void Renderer_V2::render(const GraphicState_t & aState)
{
    TIME_RECURRING_GL("Render", renderer::GpuPrimitiveGen, renderer::DrawCalls);

    // Stream the instance buffer data
    // This maps each renderer::Object to the array of its instances data (allowing instanced rendering)
    std::map<renderer::Handle<const renderer::Object>,
            // TODO Ad 2024/03/26 #RV2 change the instance type to something hosted in renderer_V2
             std::vector<snac::PoseColorSkeleton>> sortedModels;

    auto sortModelEntry = BEGIN_RECURRING_GL("Sort_meshes_and_animate");
    // The "ideal" semantic for the paletter buffer is a local,
    // but we do not want to reallocate dynamic data each frame. So clear between runs.
    mRiggingPalettesBuffer.clear();
    for (const visu_V2::Entity & entity : aState.mEntities)
    {
        assert(entity.mInstance.mObject != renderer::gNullHandle 
            && "Only instances with geometry should be part of the GraphicState");
        renderer::Handle<const renderer::Object> object = entity.mInstance.mObject;

        // Offset to the matrix palette that will be computed by this iteration
        GLuint matrixPaletteOffset = (GLuint)mRiggingPalettesBuffer.size();

        if(auto rigAnimation = entity.mAnimationState.mAnimation;
           rigAnimation != renderer::gNullHandle)
        {
            {
                TIME_RECURRING_GL("Prepare_joint_matrices");

                //entity.mInstance->mAnimationState =
                auto posedNodes =
                    renderer::animate(*rigAnimation,
                                      entity.mAnimationState.mParameterValue,
                                      object->mAnimatedRig->mRig.mJointTree);

                object->mAnimatedRig->mRig.computeJointMatrices(
                    std::back_inserter(mRiggingPalettesBuffer),
                    posedNodes);
            }
        }

        sortedModels[object].push_back(
            snac::PoseColorSkeleton{
                .pose = static_cast<math::AffineMatrix<4, float>>(entity.mInstance.mPose),
                // TODO #RV2: No need to go to SDR, we can directly stream hdr floats
                .albedo = to_sdr(entity.mColor),
                .matrixPaletteOffset = matrixPaletteOffset,
            });
    }

    // Load the matrix palettes UBO with all palettes computed during the loop
    renderer::proto::load(mJointMatrices, std::span{mRiggingPalettesBuffer}, graphics::BufferHint::StreamDraw);

    END_RECURRING_GL(sortModelEntry);

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

    // Still used by the parts handled by renderer V1 (atm: text, debug render)
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
        static const renderer::StringKey pass = "forward";

        //std::function<void(const renderer::Object &, const snac::PoseColorSkeleton & aEntity)> recurseNodes;
        auto renderObjectInstance = 
            [&, this]
            (const renderer::Object & aObject, const snac::PoseColorSkeleton & aInstance)
        {
            using renderer::gl;

            for(const renderer::Part & part : aObject.mParts)
            {
                // TODO Ad 2024/02/20: #perf #azdo pre-upload the buffer with all poses and albedos(or distinct buffer for albedos?)
                // And only upload an index per instance (actually, all the instance data has to be pre-uploaded for azdo)
                SnacGraph::InstanceData instanceData{
                    aInstance.pose,
                    aInstance.albedo,
                    (GLuint)part.mMaterial.mPhongMaterialIdx,
                };
                renderer::proto::loadSingle(*getBufferView(mRendererToKeep.mRenderGraph.mInstanceStream, semantic::gLocalToWorld).mGLBuffer,
                                            instanceData,          
                                            graphics::BufferHint::StreamDraw);

                if(renderer::Handle<renderer::ConfiguredProgram> configuredProgram = 
                        renderer::getProgramForPass(*part.mMaterial.mEffect, pass))
                {
                    // Materials are uploaded to the UBO by loadBinary()
                    // ViewProjection data is uploaded above, in the V1 code
                    
                    // Consolidate the UBO repository:
                    // Make a copy of the materials ubo, and append to it.
                    renderer::RepositoryUbo repositoryUbo = part.mMaterial.mContext->mUboRepo;
                    repositoryUbo[semantic::gViewProjection] = &mCameraBuffer.mViewing;
                    repositoryUbo[semantic::gJointMatrices] = &mJointMatrices;

                    renderer::setBufferBackedBlocks(configuredProgram->mProgram, repositoryUbo);

                    renderer::setTextures(configuredProgram->mProgram, part.mMaterial.mContext->mTextureRepo);
        
                    // TODO #RV2 make a block for lighting (with an array of lights)
                    graphics::setUniform(configuredProgram->mProgram, "u_AmbientColor", ambientColor); // single contribution for all lights
                    graphics::setUniform(configuredProgram->mProgram, "u_LightColor", lightColor);
                    graphics::setUniform(configuredProgram->mProgram, "u_LightPosition", lightPosition_cam);

                    renderer::Handle<graphics::VertexArrayObject> vao =
                        renderer::getVao(*configuredProgram, part, mRendererToKeep.mStorage);

                    graphics::ScopedBind vaoScope{*vao};
                    graphics::ScopedBind programScope{configuredProgram->mProgram.mProgram};

                    //
                    // Set the pipeline state
                    //
                    // TODO to be handled by Render Graph passes
                    glEnable(GL_CULL_FACE);
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
        };

        // TODO Ad 2024/03/27: #RV2 #azdo Consolidate draw calls.
        // Currently draw each-instance of each-object in its own draw call
        TIME_RECURRING_GL("Draw_meshes", renderer::DrawCalls);
        for (const auto & [object, instances] : sortedModels)
        {
            for (const auto & instance : instances)
            {
                renderObjectInstance(*object, instance);
            }
        }
    }

    //
    // Text
    //
    {
        // TODO Ad 2024/04/03: #profiling currently the count of draw calls is not working (returning 0)
        // because we currently have to explicity instrument the draw calls (making it easy to miss some)
        TIME_RECURRING_GL("Draw_texts", renderer::DrawCalls);

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
        TIME_RECURRING_GL("Draw_debug", renderer::DrawCalls);
        mDebugRenderer.render(aState.mDebugDrawList, mRendererToDecomission, programSetup);
    }
}

} // namespace snacgame
} // namespace ad
