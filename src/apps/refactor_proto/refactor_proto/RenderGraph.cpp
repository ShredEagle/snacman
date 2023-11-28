#include "RenderGraph.h"

#include "Cube.h"
#include "GlApi.h"
#include "Json.h"
#include "Loader.h"
#include "Logging.h"
#include "Pass.h"
#include "Profiling.h"

#include <handy/vector_utils.h>

#include <imguiui/ImguiUi.h>

#include <math/Transformations.h>
#include <math/Vector.h>
#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <renderer/BufferLoad.h>
#include <renderer/FrameBuffer.h>
#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#define NOMINMAX
#include "../../../libs/snac-renderer/snac-renderer/3rdparty/nvtx/include/nvtx3/nvtx3.hpp"

#include <array>
#include <fstream>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 420

    in vec3 ve_Position_local;
    in vec4 ve_Color;

    in uint in_ModelTransformIdx;

    layout(std140, binding = 1) uniform ViewBlock
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

constexpr math::Degree<float> gInitialVFov{50.f};
constexpr float gNearZ{-0.1f};
constexpr float gMinFarZ{-25.f};

/// @brief The orbitalcamera is moved away by the largest model dimension times this factor.
[[maybe_unused]] constexpr float gRadialDistancingFactor{1.5f};
/// @brief The far plane will be at least at this factor times the model depth.
constexpr float gDepthFactor{20.f};

namespace {

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
            {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(std::string{gVertexShader}, "RenderGraph.cpp")},
            {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(gFragmentShader, "RenderGraph.cpp")},
        };
        aStorage.mPrograms.push_back(ConfiguredProgram{
            .mProgram = IntrospectProgram{
                il,
                "simple-RenderGraph.cpp",
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
                        {"pass", "depth"},
                    },
                    .mConfiguredProgram = &aStorage.mPrograms.back(),
                }
            )},
            .mName = "simple-effect",
        };
        aStorage.mEffects.push_back(std::move(effect));

        return &aStorage.mEffects.back();
    }


    void populatePartList(PartList & aPartList, const Node & aNode, const Pose & aParentPose, const Material * aMaterialOverride)
    {
        const Instance & instance = aNode.mInstance;
        const Pose & localPose = instance.mPose;
        Pose absolutePose = aParentPose.transform(localPose);

        if(instance.mMaterialOverride)
        {
            aMaterialOverride = &*instance.mMaterialOverride;
        }

        if(Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            for(const Part & part: object->mParts)
            {
                const Material * material =
                    aMaterialOverride ? aMaterialOverride : &part.mMaterial;

                aPartList.mParts.push_back(&part);
                aPartList.mMaterials.push_back(material);
                // pushed after
                aPartList.mTransformIdx.push_back((GLsizei)aPartList.mInstanceTransforms.size());
            }

            aPartList.mInstanceTransforms.push_back(
                math::trans3d::scaleUniform(absolutePose.mUniformScale)
                * math::trans3d::translate(absolutePose.mPosition));
        }

        for(const auto & child : aNode.mChildren)
        {
            populatePartList(aPartList, child, absolutePose, aMaterialOverride);
        }
    }


} // unnamed namespace


PartList Scene::populatePartList() const
{
    PROFILER_SCOPE_RECURRING_SECTION("populate_draw_list", CpuTime);

    PartList partList;
    renderer::populatePartList(partList, mRoot, mRoot.mInstance.mPose, 
                               mRoot.mInstance.mMaterialOverride ? &*mRoot.mInstance.mMaterialOverride : nullptr);
    return partList;
}


enum class EscKeyBehaviour
{
    Ignore,
    Close,
};

//TODO Ad 2023/07/26: Move to the glfw app library,
// only register the subset actually provided by T_callbackProvider (so it does not need to implement them all)
template <class T_callbackProvider>
void registerGlfwCallbacks(graphics::AppInterface & aAppInterface,
                           T_callbackProvider & aProvider,
                           EscKeyBehaviour aEscBehaviour,
                           const imguiui::ImguiUi * aImguiUi)
{
    using namespace std::placeholders;

    aAppInterface.registerMouseButtonCallback(
        [aImguiUi, &aProvider](int button, int action, int mods, double xpos, double ypos)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackMouseButton(button, action, mods, xpos, ypos);
            }
        });
    aAppInterface.registerCursorPositionCallback(
        [aImguiUi, &aProvider](double xpos, double ypos)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackCursorPosition(xpos, ypos);
            }
        });
    aAppInterface.registerScrollCallback(
        [aImguiUi, &aProvider](double xoffset, double yoffset)
        {
            if(!aImguiUi->isCapturingMouse())
            {
                aProvider.callbackScroll(xoffset, yoffset);
            }
        });

    switch(aEscBehaviour)
    {
        case EscKeyBehaviour::Ignore:
            aAppInterface.registerKeyCallback([aImguiUi, &aProvider](int key, int scancode, int action, int mods)
            {
                if(!aImguiUi->isCapturingKeyboard())
                {
                    aProvider.callbackKeyboard(key, scancode, action, mods);
                }
            });
            break;
        case EscKeyBehaviour::Close:
            aAppInterface.registerKeyCallback(
                [&aAppInterface, &aProvider, aImguiUi](int key, int scancode, int action, int mods)
                {
                    if(!aImguiUi->isCapturingKeyboard())
                    {
                        // TODO would be cleaner to factorize that and the ApplicationGlfw::default_key_callback
                        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
                        {
                            aAppInterface.requestCloseApplication();
                        }
                        aProvider.callbackKeyboard(key, scancode, action, mods);
                    }
                });
            break;
    }
}


RenderGraph::RenderGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                         const std::filesystem::path & aSceneFile,
                         const imguiui::ImguiUi & aImguiUi) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    // At the moment, hardcode a maximum number
    mLoader{makeResourceFinder()},
    mCameraControl{mGlfwAppInterface->getWindowSize(),
                   gInitialVFov,
                   Orbital{2/*initial radius*/}
    },
    mGraph{mGlfwAppInterface, mStorage}
{
    registerGlfwCallbacks(*mGlfwAppInterface, mCameraControl, EscKeyBehaviour::Close, &aImguiUi);
    registerGlfwCallbacks(*mGlfwAppInterface, mFirstPersonControl, EscKeyBehaviour::Close, &aImguiUi);

    mScene = mLoader.loadScene(aSceneFile, "effects/Mesh.sefx", mGraph.mInstanceStream, mStorage);
    /*const*/Node & model = mScene.mRoot.mChildren.front();

    // TODO Ad 2023/10/03: Sort out this bit of logic: remove hardcoded sections,
    // better handle camera placement / projections scene wide
    // Setup instance and camera poses
    {
        // TODO For a "Model viewer mode", automatically handle scaling via model bounding box.
        // Note that visually this has the same effect as moving the radial camera away
        // (it is much harder when loading and navigating inside an environment)

        // move the orbital camera away, depending on the model size
        //mCameraControl.mOrbital.mSpherical.radius() = 
        //    std::max(mCameraControl.mOrbital.mSpherical.radius(),
        //             gRadialDistancingFactor * (*model.mAabb.mDimension.getMaxMagnitudeElement()));


        // TODO #camera do this only the correct setup, on each projection change
        // We waited to load the model before setting up the projection,
        // in order to set the far plane based on the model depth.
        mCamera.setupOrthographicProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mGlfwAppInterface->getWindowSize()),
            // TODO #camera
            .mViewHeight = mCameraControl.getViewHeightAtOrbitalCenter(),
            .mNearZ = gNearZ,
            .mFarZ = std::min(gMinFarZ, -gDepthFactor * model.mAabb.depth())
        });

        mCamera.setupPerspectiveProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mGlfwAppInterface->getWindowSize()),
            .mVerticalFov = gInitialVFov,
            .mNearZ = gNearZ,
            .mFarZ = std::min(gMinFarZ, -gDepthFactor * model.mAabb.depth())
        });
    }

    // Add basic shapes to the the scene
    Handle<Effect> simpleEffect = makeSimpleEffect(mStorage);
    auto [triangle, cube] = loadTriangleAndCube(mStorage, simpleEffect, mGraph.mInstanceStream);
    mScene.addToRoot(triangle);
    mScene.addToRoot(cube);
}


void RenderGraph::update(float aDeltaTime)
{
    PROFILER_SCOPE_RECURRING_SECTION("RenderGraph::update()", CpuTime);
    mFirstPersonControl.update(aDeltaTime);

    // TODO #camera: handle this when necessary
    // Update camera to match current values in orbital control.
    //mCamera.setPose(mCameraControl.mOrbital.getParentToLocal());
    //if(mCamera.isProjectionOrthographic())
    //{
    //    // Note: to allow "zooming" in the orthographic case, we change the viewed height of the ortho projection.
    //    // An alternative would be to apply a scale factor to the camera Pose transformation.
    //    changeOrthographicViewportHeight(mCamera, mCameraControl.getViewHeightAtOrbitalCenter());
    //}
    mCamera.setPose(mFirstPersonControl.getParentToLocal());
}


void RenderGraph::drawUi()
{
    mSceneGui.present(mScene);
}


void RenderGraph::render()
{
    PROFILER_SCOPE_RECURRING_SECTION("RenderGraph::render()", CpuTime, GpuTime, BufferMemoryWritten);
    nvtx3::mark("RenderGraph::render()");
    NVTX3_FUNC_RANGE();

    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    // Partial answer: the program selection is done later in preparePass (does not address camera overrides though)
    PartList partList = mScene.populatePartList();
    mGraph.renderFrame(partList, mCamera, mStorage);
}


} // namespace ad::renderer