#include "Renderer_V2.h"

#include "renderer/ScopeGuards.h"

#include <graphics/AppInterface.h>

#include <imguiui/ImguiUi.h> 
#include <imguiui/Widgets.h> 

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <platform/Filesystem.h>

#include <profiler/GlApi.h>

#include <renderer/BufferLoad.h>
#include <renderer/Uniforms.h>

// TODO #RV2 Remove V1 includes
#include <snac-renderer-V1/text/Text.h>
#include <snac-renderer-V1/Instances.h>
#include <snac-renderer-V1/Semantic.h>
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/ResourceLoad.h>

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


namespace {

    // A low-hanging optimization to prepend non-uniform scaling to a Pose
    math::AffineMatrix<4, float> poseTransformMatrix(math::Size<3, float> aScaling,
                                                     const renderer::Pose & aPose)
    {
        return math::trans3d::scale(aScaling * aPose.mUniformScale) 
            * aPose.mOrientation.toRotationMatrix()
            * math::trans3d::translate(aPose.mPosition);
    }

} // unnamed namespace


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
    // Note: there is actually no handle on Part atm...
    renderer::Handle<const renderer::Part> currentPart = renderer::gNullHandle;
    GLuint previousInstanceIdx = std::numeric_limits<GLuint>::max();

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
        else if(part == currentPart 
            // If there is a "hole" in the instance sequence (e.g. the part before had a different key)
            // we cannot continue with the same instanced command
            && instanceIdx == (previousInstanceIdx + 1))
        {
            // Should never be the case, we should not be able to enter this scope before the first command
            assert(!result.mDrawCommands.empty());

            // This can be drawn by incrementing the number of instances of current draw command.
            ++result.mDrawCommands.back().mInstanceCount;
            ++previousInstanceIdx;
            // If we are drawing the same part as before, with the same 
            continue;
        }

        const std::size_t indiceSize = graphics::getByteSize(vertexStream.mIndicesType);

        // Indirect draw commands do not accept a (void*) offset, but a number of indices (firstIndex).
        // This means the offset into the buffer must be aligned with the index size.
        assert((vertexStream.mIndexBufferView.mOffset %  indiceSize) == 0);

        result.mDrawCommands.push_back(
            renderer::DrawElementsIndirectCommand{
                .mCount = part->mIndicesCount,
                .mInstanceCount = 1, // Will be incremented if next entry allows it.
                .mFirstIndex = (GLuint)(vertexStream.mIndexBufferView.mOffset / indiceSize)
                                + part->mIndexFirst,
                .mBaseVertex = part->mVertexFirst,
                .mBaseInstance = instanceIdx,
            }
        );
        // Increment the draw count of current DrawCall, since we just added a DrawCommand to it.
        ++result.mCalls.back().mDrawCount;
        
        // Update inter-iteration variables
        currentPart = part;
        previousInstanceIdx = instanceIdx;
    }

    return result;
}
   

void SnacGraph::drawInstancedDirect(const renderer::Object & aObject,
                                    unsigned int & aBaseInstance,
                                    const GLsizei aInstancesCount,
                                    renderer::StringKey aPass,
                                    renderer::Storage & aStorage)
{
    using renderer::gl;

    // ATTENTION: this logic assumes there is no override of the material for a part:
    // A given part **always** use the same material, in all entities.
    for(const renderer::Part & part : aObject.mParts)
    {
        if(renderer::Handle<renderer::ConfiguredProgram> configuredProgram = 
                renderer::getProgramForPass(*part.mMaterial.mEffect, aPass))
        {
            // Materials are uploaded to the UBO by loadBinary()
            // ViewProjection data should already be loaded by the calling code.
            
            // Consolidate the UBO repository:
            // Make a copy of the materials ubo, and append to it.
            renderer::RepositoryUbo repositoryUbo = part.mMaterial.mContext->mUboRepo;
            for(const auto & [semantic, ubo] : mGraphUbos.mUboRepository)
            {
                repositoryUbo[semantic] = ubo;
            }

            renderer::setBufferBackedBlocks(configuredProgram->mProgram, repositoryUbo);

            renderer::setTextures(configuredProgram->mProgram, part.mMaterial.mContext->mTextureRepo);

            renderer::Handle<graphics::VertexArrayObject> vao =
                renderer::getVao(*configuredProgram, part, aStorage);

            graphics::ScopedBind vaoScope{*vao};
            graphics::ScopedBind programScope{configuredProgram->mProgram.mProgram};

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


void SnacGraph::renderFrame(const visu_V2::GraphicState & aState, renderer::Storage & aStorage)
{
    using renderer::gl;

    // This maps each renderer::Object to the array of its entity data.
    // This is sorting each game entity by "visual model" (== renderer::Object), 
    // associating each model to all the entities using it (represented by the EntityData required for rendering).
    // TODO Ad 2024/04/04: #perf #dod There might be a way to store all EntityData contiguously directly,
    // making it easier to load them into a GL buffer at once. But we might loose spatial coherency when accessing this buffer
    // from shaders (since the EntityData might not be sorted by Object anymore.)
    std::map<renderer::Handle<const renderer::Object>,
             std::vector<SnacGraph::EntityData>> sortedModels;

    // TODO Ad 2024/04/04: #culling This will not be true anymore when we do no necessarilly render the whole graphic state.
    const std::size_t totalEntities = aState.mEntities.size();

    //
    // Populate sortedModels and the global matrix palette
    //
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
                .mModelTransform = poseTransformMatrix(entity.mInstanceScaling, entity.mInstance.mPose),
                .mColorFactor = entity.mColor,
                .mMatrixPaletteOffset = matrixPaletteOffset,
            });
    }

    //
    // Load the matrix palettes UBO with all palettes computed during the loop
    //
    renderer::proto::load(mGraphUbos.mJointMatricesUbo,
                          std::span{mRiggingPalettesBuffer},
                          graphics::BufferHint::StreamDraw);

    END_RECURRING_GL(sortModelEntry);


    SnacGraph::PartList partList;

    //
    // Load the EntityData buffer (UBO) and the Instance buffer (instanced Vertex attribute)
    // and populate the PartList
    //
    {
        // TODO Ad 2024/04/04: Should we use SSBO for this kind of "unbound" data?
        assert(totalEntities <= 512
            && "Currently, the shader Uniform Block array is 512.");

        graphics::ScopedBind boundEntitiesUbo{mGraphUbos.mEntitiesUbo};
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

    // Load the instance buffer, at once.
    renderer::proto::load(*getBufferView(mInstanceStream, semantic::gEntityIdx).mGLBuffer,
                            std::span{mInstanceBuffer},          
                            graphics::BufferHint::StreamDraw);

    //
    // Load lighting UBO
    //
    static const math::AffineMatrix<4, GLfloat> worldToLight = 
        math::trans3d::rotateX(math::Degree<float>{60.f})
        * math::trans3d::translate<GLfloat>({0.f, 2.5f, -16.f});

    math::Position<3, GLfloat> lightPosition_cam = 
        (math::homogeneous::makePosition(math::Position<3, GLfloat>::Zero()) // light position in light space is the origin
        * worldToLight.inverse()
        * aState.mCamera.getParentToCamera()).xyz();

    math::hdr::Rgb_f lightColor = to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::hdr::Rgb_f ambientColor = math::hdr::Rgb_f{0.4f, 0.4f, 0.4f};

    {
        SnacGraph::LightingData lightingData{
            .mAmbientColor = ambientColor,
            .mLightPosition_c = lightPosition_cam,
            .mLightColor = lightColor,
        };

        graphics::ScopedBind boundLightingUbo{mGraphUbos.mLightingUbo};
        // Orphan the buffer, and set it to the current size 
        //(TODO: might be good to keep a separate counter to never shrink it)
        gl.BufferData(GL_UNIFORM_BUFFER,
                        sizeof(SnacGraph::LightingData),
                        &lightingData,
                        GL_STREAM_DRAW);
    }

    //
    // Drawing
    // 
    static const renderer::StringKey pass = "forward";

    TIME_RECURRING_GL("Draw_meshes", renderer::DrawCalls);

    // TODO: set the framebuffer / render target

    // TODO: clear

    // Set the pipeline state
    // TODO to be handled by Render Graph passes
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if(!gMultiIndirectDraw)
    {
        // Instanced single-draw, direct

        // Global instance index, in the instance buffer.
        unsigned int baseInstance = 0;
        for (const auto & [object, entities] : sortedModels)
        {
            // For each Part, there is one instance per entity 
            // (so the instances-count, which is constant accross an object, is the number of entities).
            drawInstancedDirect(*object,
                                baseInstance,
                                (GLsizei)entities.size(),
                                pass,
                                aStorage);
        }
    }
    else
    {
        // Multi-draw indirect

        // Could be done only once for the program duration, as long as only one buffer is ever used here.
        // TODO Ad 2024/04/09: #staticentities We might actually want several, for example one that is constant
        // between frames and contain the draw commands for static geometry.
        gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER, mIndirectBuffer);
        
        renderer::PassCache passCache = 
            SnacGraph::preparePass(pass, partList, aStorage);

        // Load the draw indirect buffer with command generated in preparePass()
        renderer::proto::load(mIndirectBuffer,
                            std::span{passCache.mDrawCommands},
                            graphics::BufferHint::StreamDraw);

        static const renderer::RepositoryTexture gDummyTextureRepository;

        DISABLE_PROFILING_GL;
        renderer::draw(passCache,
                       mGraphUbos.mUboRepository,
                       gDummyTextureRepository);
        ENABLE_PROFILING_GL;
    }
}


void SnacGraph::renderDebugFrame(const snac::DebugDrawer::DrawList & aDrawList,
                                 renderer::Storage & aStorage)
{
    mDebugRenderer.render(aDrawList, mGraphUbos.mUboRepository, aStorage);
}


Renderer_V2::Renderer_V2(graphics::AppInterface & aAppInterface,
                         resource::ResourceFinder & aResourcesFinder,
                         arte::FontFace aDebugFontFace) :
    mAppInterface{aAppInterface},
    // TODO Ad 2024/04/17: #decomissionRV1 #resources This temporary Loader "just to get it to work" is smelly
    // but we need to address the resource management design issue to know what to do instead...
    // (This is a hack to port snacgame to DebugRenderer V2.)
    // Hopefully it gets replaced by a more sane approach to resource management after V1 is entirely decomissionned.
    mRendererToKeep{renderer::Loader{.mFinder = aResourcesFinder}}
{}


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
        ImGui::Begin("ForwardShadow");
        ImGui::Text("TODO for Renderer V2");
        ImGui::End();
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

    // TODO move that up into the specialized classes
    // Load the viewing projection data
    renderer::proto::loadSingle(
        mRendererToKeep.mRenderGraph.mGraphUbos.mViewingProjectionUbo,
        renderer::GpuViewProjectionBlock{aState.mCamera},
        graphics::BufferHint::StreamDraw);

    if (mControl.mRenderModels)
    {
        mRendererToKeep.mRenderGraph.renderFrame(aState, mRendererToKeep.mStorage);
    }

    const math::Size<2, int> framebufferSize = mAppInterface.getFramebufferSize();

    //
    // Text
    //
    {
        // TODO Ad 2024/04/03: #profiling currently the count of draw calls is not working (returning 0)
        // because we currently have to explicity instrument the draw calls (making it easy to miss some)
        TIME_RECURRING_GL("Draw_texts", renderer::DrawCalls);

        //// 3D world text
        //if (mControl.mRenderText)
        //{
        //    // This text should be occluded by geometry in front of it.
        //    // WARNING: (smelly) the text rendering disable depth writes,
        //    //          so it must be drawn last to occlude other visuals in the 3D world.
        //    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);
        //    renderText(aState.mTextWorldEntities, programSetup);
        //}

        // For the screen space text, the viewing transform is composed as follows:
        // The world-to-camera is identity
        // The projection is orthographic, mapping framebuffer resolution (with origin at screen center) to NDC.
        snac::ProgramSetup programSetup{
            .mUniforms{
                {
                    snac::Semantic::ViewingMatrix,
                    math::trans3d::orthographicProjection(
                        math::Box<float>{
                            {-static_cast<math::Position<2, float>>(framebufferSize) / 2, -1.f},
                            {static_cast<math::Size<2, float>>(framebufferSize), 2.f}
                        })
                }}
        };

        if (mControl.mRenderText)
        {
            renderText(aState.mTextScreenEntities, programSetup);
        }
    }

    if (mControl.mRenderDebug)
    {
        TIME_RECURRING_GL("Draw_debug", renderer::DrawCalls);
        mRendererToKeep.mRenderGraph.renderDebugFrame(aState.mDebugDrawList, mRendererToKeep.mStorage);
    }
}

} // namespace snacgame
} // namespace ad
