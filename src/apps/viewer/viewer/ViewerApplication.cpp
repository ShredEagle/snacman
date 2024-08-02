#include "ViewerApplication.h"

#include "EnvironmentUtilities.h"
#include "Json.h"
#include "Logging.h"
#include "PassViewer.h"


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
#include <snac-renderer-V2/Semantics.h>
#include <snac-renderer-V2/debug/DebugDrawing.h>

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


    Handle<Effect> makeSimpleEffect(Storage & aStorage)
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


ViewerApplication::ViewerApplication(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                                     const std::filesystem::path & aSceneFile,
                                     const imguiui::ImguiUi & aImguiUi) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    // At the moment, hardcode a maximum number
    mLoader{makeResourceFinder()},
    mCameraSystem{mGlfwAppInterface, &aImguiUi, CameraSystem::Control::FirstPerson},
    mGraph{mGlfwAppInterface->getFramebufferSize(), mStorage, mLoader},
    // Track the size of the default framebuffer, which will be the destination of mGraph::render().
    mFramebufferSizeListener{mGlfwAppInterface->listenFramebufferResize(
        std::bind(&TheGraph::resize, &mGraph, std::placeholders::_1))}
{
    if(aSceneFile.extension() == ".sew")
    {
        mScene = loadScene(aSceneFile, mGraph.mInstanceStream, mLoader, mStorage);
    }
    else
    {
        throw std::invalid_argument{"Unsupported filetype: " + aSceneFile.extension().string()};
    }

    if(const auto & children = mScene.mRoot.mChildren;
       children.size() == 1)
    {
        SELOG(info)("Model viewer mode (inferred from single node)");
        mCameraSystem.setupAsModelViewer(children.front().mAabb);
    }

    // Add basic shapes to the the scene
    Handle<Effect> simpleEffect = makeSimpleEffect(mStorage);
    auto [triangle, cube] = loadTriangleAndCube(mStorage, simpleEffect, mGraph.mInstanceStream);
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
            .mMap = texture,
            .mMapFile = std::move(aPath),
        };
        aApp.mGraph.mTextureRepository[semantic::gEnvironmentTexture] = texture;
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
    // That approach is smelly: we use the Environment to compute an value we will set on the Environment
    mScene.mEnvironment->mFilteredRadiance =
        filterEnvironmentMapSpecular(*mScene.mEnvironment, mGraph, mStorage, gFilteredRadianceSide);
    glObjectLabel(GL_TEXTURE, *mScene.mEnvironment->mFilteredRadiance, -1, "filtered_radiance_env");
    mGraph.mTextureRepository[semantic::gFilteredRadianceEnvironmentTexture] = 
        mScene.mEnvironment->mFilteredRadiance;

    mScene.mEnvironment->mIntegratedBrdf =
        integrateEnvironmentBrdf(mStorage, gIntegratedBrdfSide, mLoader);
    glObjectLabel(GL_TEXTURE, *mScene.mEnvironment->mIntegratedBrdf, -1, "integrated_env_brdf");
    mGraph.mTextureRepository[semantic::gIntegratedEnvironmentBrdf] = 
        mScene.mEnvironment->mIntegratedBrdf;

    mScene.mEnvironment->mFilteredIrradiance =
        filterEnvironmentMapDiffuse(*mScene.mEnvironment, mGraph, mStorage, gFilteredIrradianceSide);
    glObjectLabel(GL_TEXTURE, *mScene.mEnvironment->mFilteredIrradiance, -1, "filtered_irradiance_env");
    mGraph.mTextureRepository[semantic::gFilteredIrradianceEnvironmentTexture] = 
        mScene.mEnvironment->mFilteredIrradiance;
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


void showPointLights(const LightsData & aLights)
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

    mCameraSystem.update(aTime.mDeltaDuration);
}


void ViewerApplication::drawUi(const renderer::Timing & aTime)
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
                mGraph,
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

    mGraphGui.present(mGraph);

    static bool gShowScene = false;
    if(imguiui::addCheckbox("Scene", gShowScene))
    {
        ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar);

        mCameraGui.presentSection(mCameraSystem);
        mSceneGui.presentSection(mScene, aTime);

        ImGui::End();
    }
}


void ViewerApplication::render()
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "ViewerApplication::render()", CpuTime, GpuTime, BufferMemoryWritten);
    nvtx3::mark("ViewerApplication::render()");
    NVTX3_FUNC_RANGE();

    mGraph.renderFrame(mScene,
                       mCameraSystem.mCamera,
                       mScene.getLightsInCamera(mCameraSystem.mCamera,
                                                !mSceneGui.getOptions().mAreDirectionalLightsCameraSpace),
                       mStorage,
                       false,
                       graphics::FrameBuffer::Default());

    mGraph.renderDebugDrawlist(snac::DebugDrawer::EndFrame(), mStorage);
}


} // namespace ad::renderer