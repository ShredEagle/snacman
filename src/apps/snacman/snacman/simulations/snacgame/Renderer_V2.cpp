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

    // TODO Ad 2024/04/09: This is a bit smelly.
    // Add the viewing block to the graph ubo repo.
    mRendererToKeep.mRenderGraph.mGraphUbos.mUboRepository[semantic::gViewProjection] = 
        &mCameraBuffer.mViewing;
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


renderer::PassCache SnacGraph::preparePass(renderer::StringKey aPass, 
                                           const PartList & aPartList,
                                           renderer::Storage & aStorage)
{
    TIME_RECURRING_GL("prepare_pass");

    renderer::DrawEntryHelper helper;
    std::vector<renderer::PartDrawEntry> entries = 
        helper.generateDrawEntries(aPass, aPartList, aStorage);

    //
    // Sort the entries
    //
    {
        TIME_RECURRING_GL("sort_draw_entries");
        // Since the PartList is sorted by Part, the PartDrawEntries will.
        // By using a stable sort, we guarantee that all instance of a given part **with the same key**
        // will still be adjacent, and in the same order, allowing some (restricted) instanced rendering.
        // Note: the restriction comes from the fact that sorting might create holes 
        //       in the pre-established instance buffer.
        std::stable_sort(entries.begin(), entries.end());
    }

    //
    // Traverse the sorted array to generate the actual draw commands and draw calls
    //
    renderer::PassCache result;
    renderer::PartDrawEntry::Key drawKey = renderer::PartDrawEntry::gInvalidKey;

    for(const renderer::PartDrawEntry & entry : entries)
    {
        renderer::Handle<const renderer::Part> part = aPartList.mParts[entry.mPartListIdx];
        const renderer::VertexStream & vertexStream = *part->mVertexStream;
        GLuint instanceIdx = aPartList.mInstanceIdx[entry.mPartListIdx];

        // If true: this is the start of a new DrawCall
        if(entry.mKey != drawKey)
        {
            // Record the new drawkey
            drawKey = entry.mKey;
            // Push the new DrawCall
            result.mCalls.push_back(helper.generateDrawCall(entry, *part, vertexStream));
        }
        
        // Increment the draw count of current DrawCall:
        // Since we are drawing a single instance with each command (i.e. 1 DrawElementsIndirectCommand for each Part)
        // we increment the drawcount for each Part (so, with each PartDrawEntry)
        ++result.mCalls.back().mDrawCount;

        const std::size_t indiceSize = graphics::getByteSize(vertexStream.mIndicesType);

        // Indirect draw commands do not accept a (void*) offset, but a number of indices (firstIndex).
        // This means the offset into the buffer must be aligned with the index size.
        assert((vertexStream.mIndexBufferView.mOffset %  indiceSize) == 0);

        result.mDrawCommands.push_back(
            renderer::DrawElementsIndirectCommand{
                .mCount = part->mIndicesCount,
                // TODO try with limited instancing
                .mInstanceCount = 1,
                .mFirstIndex = (GLuint)(vertexStream.mIndexBufferView.mOffset / indiceSize)
                                + part->mIndexFirst,
                .mBaseVertex = part->mVertexFirst,
                .mBaseInstance = instanceIdx,
            }
        );
    }

    return result;
}
                                            

void Renderer_V2::render(const GraphicState_t & aState)
{
    using renderer::gl;

    TIME_RECURRING_GL("Render", renderer::GpuPrimitiveGen, renderer::DrawCalls);

    // This maps each renderer::Object to the array of its entity data.
    // This is sorting each game entity by "visual model" (== renderer::Object), 
    // associating each model to all the entities using it (represented by the EntityData required for rendering).
    // TODO Ad 2024/04/04: #perf #dod There might be a way to store all EntityData contiguously directly,
    // making it easier to load them into a GL buffer at once. But we might loose spatial coherency when accessing this buffer
    // from shaders (since the EntityData might not be sorted by Object anymore.)
    std::map<renderer::Handle<const renderer::Object>,
             std::vector<SnacGraph::EntityData>> sortedModels;

    // TODO Ad 2024/04/04: #culling This will not be true anymore when we do no necessarilly render the whole graphic state.
    std::size_t totalEntities = aState.mEntities.size();

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
            SnacGraph::EntityData{
                .mModelTransform = static_cast<math::AffineMatrix<4, float>>(entity.mInstance.mPose),
                // TODO #RV2: No need to go to SDR, we can directly stream hdr floats
                //.mAlbedo = to_sdr(entity.mColor),
                .mMatrixPaletteOffset = matrixPaletteOffset,
            });
    }

    // Load the matrix palettes UBO with all palettes computed during the loop
    renderer::proto::load(mRendererToKeep.mRenderGraph.mGraphUbos.mJointMatricesUbo,
                          std::span{mRiggingPalettesBuffer},
                          graphics::BufferHint::StreamDraw);

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
            {snac::BlockSemantic::JointMatrices, &mRendererToKeep.mRenderGraph.mGraphUbos.mJointMatricesUbo},
        }
    };

    if (mControl.mRenderModels)
    {
        SnacGraph::PartList partList;

        // Populate the EntityData buffers
        {
            // TODO Ad 2024/04/04: Should we use SSBO for this kind of "unbound" data?
            assert(totalEntities <= 512
                && "Currently, the shader Uniform Block array is 512.");

            graphics::ScopedBind boundEntitiesUbo{mRendererToKeep.mRenderGraph.mGraphUbos.mEntitiesUbo};
            // Orphan the buffer, and set it to the current size 
            //(TODO: might be good to keep a separate counter to never shrink it)
            gl.BufferData(GL_UNIFORM_BUFFER,
                          sizeof(SnacGraph::EntityData) * totalEntities,
                          nullptr,
                          GL_STREAM_DRAW);

            mInstanceBuffer.clear();
            GLintptr entityDataOffset = 0;
            GLuint entityIdxOffset = 0;
            GLuint instanceIdx = 0;
            for (const auto & [object, entities] : sortedModels)
            {
                // This implementation relies on multiple loads, 
                // maybe it is a good candidate for buffer mapping? (glMapBufferRange())
                GLsizeiptr size = entities.size() * sizeof(SnacGraph::EntityData);
                gl.BufferSubData(
                    GL_UNIFORM_BUFFER,
                    entityDataOffset,
                    size,
                    entities.data());

                entityDataOffset += size;

                // We load all the required entities indices for **each Part**,
                // before moving on to the next Part to load the same entities.
                // This way, we can do instanced rendering on Parts 
                // (the data for all instances of a given part is contiguous).
                for(const renderer::Part & part : object->mParts)
                {
                    for(GLuint entityGlobalIdx = entityIdxOffset;
                        entityGlobalIdx != entityIdxOffset + entities.size();
                        ++entityGlobalIdx)
                    {
                        mInstanceBuffer.push_back(SnacGraph::InstanceData{
                            .mEntityIdx = entityGlobalIdx,
                            .mMaterialIdx = (GLuint)part.mMaterial.mPhongMaterialIdx,
                        });

                        // In the partlist, keep track of which part correspond to which entry
                        // in the instance buffer, as well as the associated material
                        // (so we can look up the Program for a given pass)
                        partList.push_back(&part, &part.mMaterial, instanceIdx);
                        ++instanceIdx;
                    }
                }
                entityIdxOffset += (GLuint)entities.size();
            }
        }

        renderer::proto::load(*getBufferView(mRendererToKeep.mRenderGraph.mInstanceStream, semantic::gEntityIdx).mGLBuffer,
                              std::span{mInstanceBuffer},          
                              graphics::BufferHint::StreamDraw);

        //
        // Load lighting UBO
        //
        {
            SnacGraph::LightingData lightingData{
                .mAmbientColor = ambientColor,
                .mLightPosition_c = lightPosition_cam,
                .mLightColor = lightColor,
            };

            graphics::ScopedBind boundLightingUbo{mRendererToKeep.mRenderGraph.mGraphUbos.mLightingUbo};
            // Orphan the buffer, and set it to the current size 
            //(TODO: might be good to keep a separate counter to never shrink it)
            gl.BufferData(GL_UNIFORM_BUFFER,
                          sizeof(SnacGraph::LightingData),
                          &lightingData,
                          GL_STREAM_DRAW);
        }

        static const renderer::StringKey pass = "forward";

        auto renderPartsInstanced = 
            [&, this]
            (const renderer::Object & aObject, unsigned int & aBaseInstance, const GLsizei aInstancesCount)
        {
            using renderer::gl;

            // Important: this logic assumes there is no override of the material for a part:
            // A given part **always** use the same material, in all entities.
            for(const renderer::Part & part : aObject.mParts)
            {
                if(renderer::Handle<renderer::ConfiguredProgram> configuredProgram = 
                        renderer::getProgramForPass(*part.mMaterial.mEffect, pass))
                {
                    // Materials are uploaded to the UBO by loadBinary()
                    // ViewProjection data is uploaded above, in the V1 code
                    
                    // Consolidate the UBO repository:
                    // Make a copy of the materials ubo, and append to it.
                    renderer::RepositoryUbo repositoryUbo = part.mMaterial.mContext->mUboRepo;
                    repositoryUbo[semantic::gViewProjection] = &mCameraBuffer.mViewing;
                    repositoryUbo[semantic::gJointMatrices] = &mRendererToKeep.mRenderGraph.mGraphUbos.mJointMatricesUbo;
                    repositoryUbo[semantic::gEntities] = &mRendererToKeep.mRenderGraph.mGraphUbos.mEntitiesUbo;
                    repositoryUbo[semantic::gLighting] = &mRendererToKeep.mRenderGraph.mGraphUbos.mLightingUbo;

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

                    gl.DrawElementsInstancedBaseVertexBaseInstance(
                        part.mPrimitiveMode,
                        part.mIndicesCount,
                        part.mVertexStream->mIndicesType,
                        (void*)((size_t)part.mIndexFirst * graphics::getByteSize(part.mVertexStream->mIndicesType)),
                        aInstancesCount,
                        part.mVertexFirst,
                        aBaseInstance
                    );
                }
                aBaseInstance += aInstancesCount;
            }
        };

        // TODO Ad 2024/03/27: #RV2 #azdo Consolidate draw calls.
        // Currently draw each-instance of each-object in its own draw call
        TIME_RECURRING_GL("Draw_meshes", renderer::DrawCalls);

#if 0
        // The single & direct draw (instanced)

        // Global instance index, in the instance buffer.
        unsigned int baseInstance = 0;
        for (const auto & [object, entities] : sortedModels)
        {
            // For each Part, there is one instance per entity 
            // (so the instances-count, which is constant accross an object, is the number of entities).
            renderPartsInstanced(*object, baseInstance, (GLsizei)entities.size());
        }
#else
        // Multi-draw indirect

        // Could be done only once for the program duration, as long as only one buffer is ever used here.
        // TODO Ad 2024/04/09: #staticentities We might actually want several, for example one that is constant
        // between frames and contain the draw commands for static geometry.
        gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER, mRendererToKeep.mRenderGraph.mIndirectBuffer);
        
        renderer::PassCache passCache = 
            SnacGraph::preparePass(pass, partList, mRendererToKeep.mStorage);

        // Load the draw indirect buffer with command generated in preparePass()
        renderer::proto::load(mRendererToKeep.mRenderGraph.mIndirectBuffer,
                              std::span{passCache.mDrawCommands},
                              graphics::BufferHint::StreamDraw);

        // TODO: set the framebuffer / render target

        // TODO: clear

        //
        // Set the pipeline state
        //
        // TODO to be handled by Render Graph passes
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        static const renderer::RepositoryTexture gDummyTextureRepository;

        //DISABLE_PROFILING_GL;
        renderer::draw(passCache,
                       mRendererToKeep.mRenderGraph.mGraphUbos.mUboRepository,
                       gDummyTextureRepository);
        //ENABLE_PROFILING_GL;
#endif
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
