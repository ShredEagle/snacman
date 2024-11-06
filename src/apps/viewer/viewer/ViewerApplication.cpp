#include "ViewerApplication.h"

#include "EnvironmentUtilities.h"
#include "Json.h"
#include "Logging.h"
#include "PassViewer.h"
#include "math/Homogeneous.h"


#include <handy/vector_utils.h>

#include <imguiui/ImguiUi.h>
#include <imguiui/Widgets.h>

#include <math/Transformations.h>
#include <math/Vector.h>
#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <profiler/GlApi.h>

#include <snac-renderer-V2/Cube.h>
#include <snac-renderer-V2/Profiling.h>
#include <snac-renderer-V2/RendererReimplement.h>
#include <snac-renderer-V2/Semantics.h>

#include <snac-renderer-V2/debug/DebugDrawing.h>

#include <snac-renderer-V2/utilities/ColorPalettes.h>
#include <snac-renderer-V2/utilities/DebugDrawUtilities.h>
#include <snac-renderer-V2/utilities/LoadUbos.h>

#include <utilities/Time.h>

// TODO #nvtx This should be handled cleanly by the profiler
#include "../../../libs/snac-renderer-V1/snac-renderer-V1/3rdparty/nvtx/include/nvtx3/nvtx3.hpp"

#include <array>
#include <fstream>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 420

    in vec3 ve_Position_local;
    in vec4 ve_Color;

    in uint in_ModelTransformIdx;

    layout(std140, binding = 1) uniform ViewProjectionBlock
    {
        mat4 worldToCamera;
        mat4 projection;
        mat4 viewingProjection;
    };

    // TODO #ssbo Use a shader storage block, due to the unbounded nature of the number of instances
    layout(std140, binding = 0) uniform LocalToWorldBlock
    {
        mat4 localToWorld[512];
    };

    out vec4 ex_Color;

    void main()
    {
        ex_Color = ve_Color;
        gl_Position = viewingProjection 
            * localToWorld[in_ModelTransformIdx] 
            * vec4(ve_Position_local, 1.f);
    }
)#";

const char * gFragmentShader = R"#(
    #version 420

    in vec4 ex_Color;

    layout(std140, binding = 2) uniform FrameBlock
    {
        float time;
    };

    out vec4 color;

    void main()
    {
        // TODO Ad 2023/10/12: #transparency This would be a nice transparency effect
        //color = vec4(ex_Color.rgb, mod(time, 1.0));
        color = vec4(ex_Color.rgb, 1.) * vec4(mod(time, 1.0));
    }
)#";


namespace {

    constexpr math::Size<3, float> gLightCubeSize{0.2f, 0.2f, 0.2f};

    resource::ResourceFinder makeResourceFinder()
    {
        filesystem::path assetConfig = platform::getExecutableFileDirectory() / "assets.json";
        if(exists(assetConfig))
        {
            Json config = Json::parse(std::ifstream{assetConfig});
            
            // This leads to an ambibuity on the path ctor, I suppose because
            // the iterator value_t is equally convertible to both filesystem::path and filesystem::path::string_type
            //return resource::ResourceFinder(config.at("prefixes").begin(),
            //                                config.at("prefixes").end());

            // Take the silly long way
            std::vector<std::string> prefixes{
                config.at("prefixes").begin(),
                config.at("prefixes").end()
            };
            std::vector<std::filesystem::path> prefixPathes;
            prefixPathes.reserve(prefixes.size());
            for (auto & prefix : prefixes)
            {
                prefixPathes.push_back(std::filesystem::canonical(prefix));
            }
            return resource::ResourceFinder(prefixPathes.begin(),
                                            prefixPathes.end());
        }
        else
        {
            return resource::ResourceFinder{platform::getExecutableFileDirectory()};
        }
    }


    // GCC complains about unused functions in unnamed namespaces
    [[maybe_unused]] Handle<Effect> makeSimpleEffect(Storage & aStorage)
    {
        aStorage.mProgramConfigs.emplace_back();
        // Compiler error (msvc) workaround, by taking the initializer list out
        std::initializer_list<IntrospectProgram::TypedShaderSource> il = {
            {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(std::string{gVertexShader}, "ViewerApplication.cpp")},
            {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(gFragmentShader, "ViewerApplication.cpp")},
        };
        aStorage.mPrograms.push_back(ConfiguredProgram{
            .mProgram = IntrospectProgram{
                il,
                "simple-ViewerApplication.cpp",
            },
            .mConfig = &aStorage.mProgramConfigs.back(),
        });

        Effect effect{
            .mTechniques{makeVector(
                Technique{
                    .mAnnotations{
                        {"pass", "forward"},
                    },
                    .mConfiguredProgram = &aStorage.mPrograms.back(),
                },
                // Use the same program at the moment for the depth pass
                Technique{
                    .mAnnotations{
                        {"pass", "depth_opaque"},
                    },
                    .mConfiguredProgram = &aStorage.mPrograms.back(),
                }
            )},
            .mName = "simple-effect",
        };
        aStorage.mEffects.push_back(std::move(effect));

        aStorage.mEffectLoadInfo.push_back(Storage::gNullLoadInfo);

        return &aStorage.mEffects.back();
    }


    void populatePartList(ViewerPartList & aPartList,
                          const Node & aNode,
                          const Pose & aParentAbsolutePose,
                          const Material * aMaterialOverride)
    {
        const Instance & instance = aNode.mInstance;
        const Pose & localPose = instance.mPose;
        Pose absolutePose = aParentAbsolutePose.transform(localPose);

        if(instance.mMaterialOverride)
        {
            aMaterialOverride = &*instance.mMaterialOverride;
        }

        if(const Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            // the default value (i.e. not rigged)
            GLsizei paletteOffset = ViewerPartList::gInvalidIdx;

            if(auto animatedRig = object->mAnimatedRig)
            {
                PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "compute_joint_matrices", CpuTime);

                // TODO Ad 2024/03/20 #perf #animation:
                // We could share the matrix palette for all no-animation (i.e. default rig pose),
                // or even for "static animations" (i.e. single keyframe, defining a static pose)
                // instead of currently always producing a matrix palette for each object with a rig.
                paletteOffset = (GLsizei)aPartList.mRiggingPalettes.size();

                const Rig & rig = animatedRig->mRig;
                aPartList.mRiggingPalettes.reserve(aPartList.mRiggingPalettes.size() + rig.countJoints());
                rig.computeJointMatrices(
                        std::back_inserter(aPartList.mRiggingPalettes), 
                        aNode.mInstance.mAnimationState ? aNode.mInstance.mAnimationState->mPosedTree 
                                                   : animatedRig->mRig.mJointTree);
            }

            for(const Part & part: object->mParts)
            {
                const Material * material =
                    aMaterialOverride ? aMaterialOverride : &part.mMaterial;

                aPartList.mParts.push_back(&part);
                aPartList.mMaterials.push_back(material);
                // pushed after
                aPartList.mTransformIdx.push_back((GLsizei)aPartList.mInstanceTransforms.size());
                aPartList.mPaletteOffset.push_back(paletteOffset);
            }

            aPartList.mInstanceTransforms.push_back(
                math::trans3d::scaleUniform(absolutePose.mUniformScale)
                * absolutePose.mOrientation.toRotationMatrix()
                * math::trans3d::translate(absolutePose.mPosition));
        }

        for(const auto & child : aNode.mChildren)
        {
            populatePartList(aPartList, child, absolutePose, aMaterialOverride);
        }
    }


    // Note: I would have liked to merge it with "populatePartList"
    // but it might be called from several "views" in a single frame, thus duplicating the debug draws...
    void debugDrawAabb(const Node & aNode,
                       const Pose & aParentAbsolutePose,
                       const Node * aHighlightedNode)
    {
        using ::ad::snac::DebugDrawer;

        const Instance & instance = aNode.mInstance;
        const Pose & localPose = instance.mPose;
        Pose absolutePose = aParentAbsolutePose.transform(localPose);

        // All AABB are expressed in the **local** space (and thus, aligned on the axis of local space).

        // For the highlighted Node, we raise the drawing level
        // This way, it is possible to filter and see only the highlighted node AABB.
        DebugDrawer::Level nodeLevel = 
            aHighlightedNode == &aNode ? DebugDrawer::Level::info : DebugDrawer::Level::debug;

        DBGDRAW(drawer::gScene, nodeLevel).addBox(
            // TODO Ad 2024/08/24: #pose This should become a simple assignment when Pose is consolidated
            snac::DebugDrawer::Entry{
                .mPosition = absolutePose.mPosition.as<math::Position>(),
                .mScaling = math::Size<3, float>{
                    absolutePose.mUniformScale,
                    absolutePose.mUniformScale,
                    absolutePose.mUniformScale},
                .mOrientation = absolutePose.mOrientation,
                .mColor = math::hdr::gBlue<float>,
            },
            aNode.mAabb);

        if(const Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            DBGDRAW_TRACE(drawer::gScene).addBox(
                // TODO Ad 2024/08/24: #pose This should become a simple assignment when Pose is consolidated
                snac::DebugDrawer::Entry{
                    .mPosition = absolutePose.mPosition.as<math::Position>(),
                    .mScaling = math::Size<3, float>{
                        absolutePose.mUniformScale,
                        absolutePose.mUniformScale,
                        absolutePose.mUniformScale},
                    .mOrientation = absolutePose.mOrientation,
                },
                object->mAabb);
        }

        for(const auto & child : aNode.mChildren)
        {
            debugDrawAabb(child, absolutePose, aHighlightedNode);
        }
    }


} // unnamed namespace


ViewerPartList Scene::populatePartList() const
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "populate_part_list", CpuTime);

    ViewerPartList partList;
    renderer::populatePartList(partList,
                               mRoot,
                               Pose{}, // Identity pose, because it represents
                                       // the pose of the canonical space in the canonical space
                               mRoot.mInstance.mMaterialOverride ? &*mRoot.mInstance.mMaterialOverride 
                                                                   : nullptr);
    return partList;
}


PrimaryView::PrimaryView(const std::shared_ptr<graphics::AppInterface> & aGlfwAppInterface, const imguiui::ImguiUi & aImguiUi, Loader & aLoader, Storage & aStorage) :
    mCameraSystem{aGlfwAppInterface, &aImguiUi, CameraSystem::Control::FirstPerson},
    mGraph{aGlfwAppInterface->getFramebufferSize(), aStorage, aLoader},
    mFramebufferSizeListener{aGlfwAppInterface->listenFramebufferResize(
        std::bind(&TheGraph::resize, &mGraph, std::placeholders::_1))}
{}


ViewerApplication::ViewerApplication(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                                     std::shared_ptr<graphics::AppInterface> aSecondViewAppInterface,
                                     const std::filesystem::path & aSceneFile,
                                     const imguiui::ImguiUi & aImguiUi,
                                     const imguiui::ImguiUi & aSecondViewImguiUi) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    mSecondViewAppInterface{std::move(aSecondViewAppInterface)},
    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    // At the moment, hardcode a maximum number
    mLoader{makeResourceFinder()},
    mSceneGui{mLoader.loadEffect("effects/Highlight.sefx", mStorage), mStorage},
    // Track the size of the default framebuffer, which will be the destination of mGraph::render().
    mDebugRenderer{mStorage, mLoader},
    mGraphShared{mLoader, mStorage},
    mPrimaryView{mGlfwAppInterface, aImguiUi, mLoader, mStorage},
    mSecondaryView{mSecondViewAppInterface, aSecondViewImguiUi, mStorage, mLoader}
{
    if(aSceneFile.extension() == ".sew")
    {
        // TODO Ad 2024/08/02: #graph sort out this instance stream stuff
        mScene = loadScene(aSceneFile, mGraphShared.mInstanceStream, mLoader, mStorage);
    }
    else
    {
        throw std::invalid_argument{"Unsupported filetype: " + aSceneFile.extension().string()};
    }

    if(const auto & children = mScene.mRoot.mChildren;
       children.size() == 1)
    {
        SELOG(info)("Model viewer mode (inferred from single node)");
        mPrimaryView.mCameraSystem.setupAsModelViewer(children.front().mAabb);
    }

    //// Add basic shapes to the the scene
    //Handle<Effect> simpleEffect = makeSimpleEffect(mStorage);
    //auto [triangle, cube] = loadTriangleAndCube(mStorage, simpleEffect, mGraph.mInstanceStream);
    // Toggle the triangle and cube in the scene
    //mScene.addToRoot(triangle);
    //mScene.addToRoot(cube);
}


namespace {

    void setEnvironmentMap(ViewerApplication & aApp, graphics::Texture aMap, Environment::Type aType, std::filesystem::path aPath)
    {
        aApp.mStorage.mTextures.push_back(std::move(aMap));
        Handle<const graphics::Texture> texture = &aApp.mStorage.mTextures.back();
        aApp.mScene.mEnvironment = Environment{
            .mType = aType,
            .mTextureRepository{ {semantic::gEnvironmentTexture, texture}, },
            .mMapFile = std::move(aPath),
        };
    }

} // unnamed namespace

void ViewerApplication::setEnvironmentCubemap(std::filesystem::path aEnvironment)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "Load environment map",
                                      CpuTime, GpuTime, BufferMemoryWritten);
    SELOG(info)("Loading environment map from cubemap strip '{}'", aEnvironment.string());

    setEnvironmentMap(*this, 
                      aEnvironment.extension() == ".dds" ?
                        loadCubemapFromDds(aEnvironment)
                        : loadCubemapFromStrip(aEnvironment),
                      //loadCubemapFromSequence(aEnvironmentStrip),
                      Environment::Cubemap,
                      aEnvironment);

    //
    // Setup the filtered environment textures
    //
    const GLint gFilteredRadianceSide = 512;
    const GLint gIntegratedBrdfSide = 512;
    const GLint gFilteredIrradianceSide = 128;

    // That approach is smelly: we use the Environment to compute a value we then assign to the Environment
    mScene.mEnvironment->mTextureRepository[semantic::gFilteredRadianceEnvironmentTexture] =
        // TODO Ad 2024/08/02: #graph get rid of the graph parameter in the filtering functions
        filterEnvironmentMapSpecular(*mScene.mEnvironment, mGraphShared, mStorage, gFilteredRadianceSide);

    mScene.mEnvironment->mTextureRepository[semantic::gIntegratedEnvironmentBrdf] =
        integrateEnvironmentBrdf(mStorage, gIntegratedBrdfSide, mLoader);

    mScene.mEnvironment->mTextureRepository[semantic::gFilteredIrradianceEnvironmentTexture] =
        filterEnvironmentMapDiffuse(*mScene.mEnvironment, mGraphShared, mStorage, gFilteredIrradianceSide);
}


void ViewerApplication::setEnvironmentEquirectangular(std::filesystem::path aEnvironmentEquirect)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "Load environment map",
                                      CpuTime, GpuTime, BufferMemoryWritten);
    SELOG(info)("Loading environment map from equirectangular map '{}'", aEnvironmentEquirect.string());

    setEnvironmentMap(*this, 
                      loadEquirectangular(aEnvironmentEquirect),
                      Environment::Equirectangular,
                      aEnvironmentEquirect);
}



constexpr std::array<math::hdr::Rgba<float>, 4> gColorRotation
{
    math::hdr::gCyan<float>,
    math::hdr::gMagenta<float>,
    math::hdr::gGreen<float>,
    math::hdr::gYellow<float>,
};


void drawJointTree(const NodeTree<Rig::Pose> & aRigTree,
                   NodeTree<Rig::Pose>::Node::Index aNodeIdx,
                   const Rig::FuturePose_type & aPose,
                   math::AffineMatrix<4, float> aModelTransform,
                   std::size_t aColorIdx = 0,
                   std::optional<math::Position<3, float>> aParentPosition = std::nullopt)
{
    using Node = NodeTree<Rig::Pose>::Node;

    const Node & node = aRigTree.mHierarchy[aNodeIdx];
    math::Position<3, float> position =
        (aPose.mGlobalPose[aNodeIdx] * aModelTransform).getAffine().as<math::Position>();

    if(aParentPosition)
    {
        DBGDRAW_INFO(drawer::gRig).addLine(*aParentPosition, position, gColorRotation[aColorIdx % gColorRotation.size()]);
    }

    if(aRigTree.hasChild(aNodeIdx))
    {
        drawJointTree(aRigTree, node.mFirstChild, aPose, aModelTransform, aColorIdx + 1, position);
    }

    if(node.mNextSibling != Node::gInvalidIndex)
    {
        drawJointTree(aRigTree, node.mNextSibling, aPose, aModelTransform, aColorIdx, aParentPosition);
    }
}


void handleAnimations(Node & aNode,
                      const Timing & aTime,
                      Pose aParentPose = {})
{
    Pose pose = aParentPose.transform(aNode.mInstance.mPose);
    if(const Object * object = aNode.mInstance.mObject)
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "handle_animations", CpuTime);
        if(auto & animationState = aNode.mInstance.mAnimationState)
        {
            assert(object->mAnimatedRig);
            const RigAnimation & animation = *animationState->mAnimation;
            animationState->mPosedTree = animate(
                animation,
                (float)std::fmod(aTime.mSimulationTimepoint - animationState->mStartTimepoint, (double)animation.mDuration),
                object->mAnimatedRig->mRig.mJointTree);
        }

        // Draw the rig (potentially posed by animation)
        if(object->mAnimatedRig)
        {
            const Rig & rig = object->mAnimatedRig->mRig;
            drawJointTree(rig.mJointTree,
                            rig.mJointTree.mFirstRoot,
                            aNode.mInstance.mAnimationState ? aNode.mInstance.mAnimationState->mPosedTree
                                                        : rig.mJointTree,
                            static_cast<math::AffineMatrix<4, float>>(pose));
        }

    }

    for(Node & child : aNode.mChildren)
    {
        handleAnimations(child, aTime, pose);
    }
}


void showPointLights(const LightsDataUser & aLights)
{
    for(const auto & pointLight : aLights.spanPointLights())
    {
        DBGDRAW_INFO(drawer::gLight).addBox(
            {
                .mPosition = pointLight.mPosition,
                .mColor = pointLight.mDiffuseColor,
            },
            math::Box<float>{-gLightCubeSize.as<math::Position>() / 2.f, gLightCubeSize});
    }
}


void PrimaryView::update(const Timing & aTime)
{
    mCameraSystem.update(aTime.mDeltaDuration);

    // Debugdraw the camera
    {
        auto cameraToWorld = mCameraSystem.mCamera.getParentToCamera().inverse();

        // Show basis
        DBGDRAW_INFO(drawer::gCamera).addBasis({
            .mPosition = cameraToWorld.getAffine().as<math::Position>(),
            .mOrientation = math::toQuaternion(cameraToWorld.getLinear()),
        });

        auto viewProjectionInverse = mCameraSystem.mCamera.assembleViewProjection().inverse();
        debugDrawViewFrustum(viewProjectionInverse, drawer::gCamera, hdr::gCameraFrustumColor);
    }
}


void ViewerApplication::update(const Timing & aTime)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "ViewerApplication::update()", CpuTime);

    snac::DebugDrawer::StartFrame();

    // Draw the Rigs joints / bones using DebugDrawer
    // Animate the rigs
    handleAnimations(mScene.mRoot, aTime);

    // handle lights
    if(mSceneGui.getOptions().mShowPointLights)
    {
        showPointLights(mScene.mLights_world);
    }

    mPrimaryView.update(aTime);
    mSecondaryView.update(aTime);

    debugDrawAabb(mScene.mRoot, Pose{}, mSceneGui.getHighlighted());
}


void ViewerApplication::drawMainUi(const renderer::Timing & aTime)
{
    if(ImGui::Button("Recompile effects"))
    {
        LapTimer timer;
        if(recompileEffects(mLoader, mStorage))
        {
            SELOG(info)("Successfully recompiled effect, took {:.3f}s.",
                        asFractionalSeconds(timer.mark()));
        }
        else
        {
            SELOG(error)("Could not recompile all effects.");
        }
    }

    auto conditionalGuard = [](bool aCondition, 
                               auto aEnter,
                               Guard::release_fun aExit) -> Guard
    {
        if(aCondition)
        {
            aEnter();
            return Guard{std::move(aExit)};
        }
        else
        {
            return Guard{[](){}};
        }
    };

    {
        Guard scopeDisable = conditionalGuard(
            !mScene.mEnvironment || mScene.mEnvironment->mType != Environment::Equirectangular,
            [](){ImGui::BeginDisabled();},
            ImGui::EndDisabled);

        if(ImGui::Button("Dump environment"))
        {
            assert(mScene.mEnvironment);
            dumpEnvironmentCubemap(
                *mScene.mEnvironment,
                mGraphShared,
                mStorage,
                mScene.mEnvironment->mMapFile.parent_path() /
                    "split-" += mScene.mEnvironment->mMapFile.filename());
        }
    }
    {
        Guard scopeDisable = conditionalGuard(
            !mScene.mEnvironment,
            [](){ImGui::BeginDisabled();},
            ImGui::EndDisabled);

        if(ImGui::Button("Highlight samples"))
        {
            float roughness = 1.f/9.f; // the first level of our 10 mipmaps
            // Normal is provided right-handed: z -> -1 "into the screen" normal
            static graphics::Texture highlights =
                highLightSamples(*mScene.mEnvironment,
                                 math::Vec<3, float>(0.f, 0.f, -1.f),
                                 roughness,
                                 mLoader);
            glObjectLabel(GL_TEXTURE, highlights, -1, "highlight-samples");
            dumpToFile(highlights,
                    mScene.mEnvironment->mMapFile.parent_path() /
                        "samples-" += 
                            mScene.mEnvironment->mMapFile.filename().replace_extension("png"),
                    0);
        }
    }

    mPrimaryView.mGraphGui.present(mPrimaryView.mGraph);

    static bool gShowScene = false;
    if(imguiui::addCheckbox("Scene", gShowScene))
    {
        ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar);

        mPrimaryView.mCameraGui.presentSection(mPrimaryView.mCameraSystem);
        mSceneGui.presentSection(mScene, aTime);

        ImGui::End();
    }

    bool showSecondWindow = !mSecondViewAppInterface->isWindowHidden();
    if(ImGui::Checkbox("Second window", &showSecondWindow))
    {
        if(showSecondWindow)
        {
            mSecondViewAppInterface->showWindow();
        }
        else
        {
            mSecondViewAppInterface->hideWindow();
        }
    }

    mDebugAndLogGui.present();
}


void ViewerApplication::drawSecondaryUi()
{
    mSecondaryView.mCameraGui.presentSection(mSecondaryView.mCameraSystem);
}


void PrimaryView::render(const Scene & aScene,
                         const ViewerPartList & aPartList,
                         bool aLightsInCameraSpace,
                         const GraphShared & aGraphShared,
                         Storage & aStorage)
{
    mGraph.renderFrame(aScene,
                       aPartList,
                       mCameraSystem.mCamera,
                       aScene.getLightsInCamera(mCameraSystem.mCamera, !aLightsInCameraSpace),
                       aGraphShared,
                       aStorage,
                       true,
                       graphics::FrameBuffer::Default());
}


void ViewerApplication::render()
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "ViewerApplication::render()", CpuTime, GpuTime, BufferMemoryWritten);
    nvtx3::mark("ViewerApplication::render()");
    NVTX3_FUNC_RANGE();

    // 
    // Prepare what is intended to be shared by all the graphs
    //

    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    // Partial answer: the program selection is done later in preparePass (does not address camera overrides though)
    ViewerPartList partList = mScene.populatePartList();

    // Use the same indirect buffer for all drawings
    // Its content will be rewritten by distinct passes though
    // With the current approach, this binding could be done only once at construction,
    // unless several "Graph" instances use distinct indirect buffers
    graphics::bind(mGraphShared.mIndirectBuffer, graphics::BufferType::DrawIndirect);
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_frame_UBOs", CpuTime, GpuTime, BufferMemoryWritten);
        loadFrameUbo(*mGraphShared.mUbos.mFrameUbo);
        // Load model transforms
        // Note: the model matrix transforms to world space, so it is independant of viewpoint.
        // (the pass indexes the transform for the objects it actually draws, so culling could still be implemented on top).
        proto::load(*mGraphShared.mUbos.mModelTransformUbo,
                    std::span{partList.mInstanceTransforms},
                    graphics::BufferHint::DynamicDraw);

        proto::load(*mGraphShared.mUbos.mJointMatrixPaletteUbo, std::span{partList.mRiggingPalettes}, graphics::BufferHint::StreamDraw);
    }

    //
    // Render the views (via their graph)
    //
    mPrimaryView.render(mScene, partList, mSceneGui.getOptions().mAreDirectionalLightsCameraSpace, mGraphShared, mStorage);

    // Not sure if we want to also include the secondary view render in the debugdrawer frame.
    // Since at the moment of writing it calls the same code, we do not include it, to avoid duplicated debug drawing.
    snac::DebugDrawer::DrawList drawList = snac::DebugDrawer::EndFrame();

    if(mSecondViewAppInterface->isWindowOnDisplay())
    {
        mSecondaryView.render(mScene, partList, mSceneGui.getOptions().mAreDirectionalLightsCameraSpace, mGraphShared, mStorage);
    }

    // TODO Ad 2024/08/23: This manual management of each window should be generalized behind a notion of "sink"
    // so we can separately control which channel/level goes to each sink (instead of completely toggling on/off)
    if(mDebugAndLogGui.mRenderToMainView)
    {
        // We reset the camera to the main view, this is a smell
        loadCameraUbo(*mGraphShared.mUbos.mViewingUbo, mPrimaryView.mCameraSystem.mCamera);
        graphics::ScopedBind boundFbo{graphics::FrameBuffer::Default()};
        renderDebugDrawlist(
            drawList,
            // Fullscreen viewport
            math::Rectangle<int>::AtOrigin(mGlfwAppInterface->getFramebufferSize()),
            // TODO Ad 2024/08/02: #graph This need to access the ubo repo in the graph is another design smell
            mGraphShared.mUbos.mUboRepository);
    }

    if(mSecondViewAppInterface->isWindowOnDisplay()
        && mDebugAndLogGui.mRenderToSecondaryView)
    {
        // We reset the camera to the secondary view
        loadCameraUbo(*mGraphShared.mUbos.mViewingUbo, mSecondaryView.mCameraSystem.mCamera);
        graphics::ScopedBind boundFbo{mSecondaryView.mDrawFramebuffer};
        renderDebugDrawlist(
            drawList,
            // Fullscreen viewport
            math::Rectangle<int>::AtOrigin(mSecondaryView.mRenderSize),
            // TODO Ad 2024/08/02: #graph This need to access the ubo repo in the graph is another design smell
            // It is needed for the viewing block, so we hope it is left at the camera position
            mGraphShared.mUbos.mUboRepository);
    }
}


void ViewerApplication::renderDebugDrawlist(const snac::DebugDrawer::DrawList & aDrawList,
                                            math::Rectangle<int> aViewport,
                                            const RepositoryUbo & aUboRepo)
{
    glViewport(aViewport.xMin(), aViewport.yMin(), aViewport.width(), aViewport.height());
    mDebugRenderer.render(aDrawList, aUboRepo, mStorage);
}


} // namespace ad::renderer
