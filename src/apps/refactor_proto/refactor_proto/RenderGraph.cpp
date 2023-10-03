#include "RenderGraph.h"

#include "Cube.h"
#include "Json.h"
#include "Loader.h"
#include "Logging.h"
#include "Profiling.h"
#include "SetupDrawing.h"

#include <handy/vector_utils.h>

#include <imguiui/ImguiUi.h>

#include <math/Transformations.h>
#include <math/Vector.h>
#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <renderer/BufferLoad.h>
#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#define NOMINMAX
#include "../../../libs/snac-renderer/snac-renderer/3rdparty/nvtx/include/nvtx3/nvtx3.hpp"

#include <array>
#include <fstream>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 420

    in vec3 v_Position_local;

    in mat4 i_LocalToWorld;

    layout(std140, binding = 1) uniform ViewBlock
    {
        mat4 worldToCamera;
        mat4 projection;
        mat4 viewingProjection;
    };

    void main()
    {
        gl_Position = viewingProjection * i_LocalToWorld * vec4(v_Position_local, 1.f);
    }
)#";

const char * gFragmentShader = R"#(
    #version 420

    layout(std140, binding = 2) uniform FrameBlock
    {
        float time;
    };

    out vec4 color;

    void main()
    {
        color = vec4(1.0, 0.5, 0.25, 1.f) * vec4(mod(time, 1.0));
    }
)#";

constexpr math::Degree<float> gInitialVFov{50.f};
constexpr float gNearZ{-0.1f};
constexpr float gMinFarZ{-25.f}; // Note: minimum in **absolute** value (i.e. the computed far Z can only be inferior to this).

/// @brief The orbitalcamera is moved away by the largest model dimension times this factor.
constexpr float gRadialDistancingFactor{1.5f};
/// @brief The far plane will be at least at this factor times the model depth.
constexpr float gDepthFactor{20.f};

constexpr std::size_t gMaxDrawInstances = 2048;

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



    //VertexStream makeTriangle(Storage & aStorage)
    //{
    //    std::array<math::Position<3, GLfloat>, 3> vertices{{
    //        { 1.f, -1.f, 0.f},
    //        { 0.f,  1.f, 0.f},
    //        {-1.f, -1.f, 0.f},
    //    }};
    //    return makeFromPositions(aStorage, std::span{vertices});
    //}

    //VertexStream makeCube(Storage & aStorage)
    //{
    //    auto cubeVertices = getExpandedCubeVertices<math::Position<3, GLfloat>>();
    //    return makeFromPositions(aStorage, std::span{cubeVertices});
    //}

    Material makeWhiteMaterial(Storage & aStorage)
    {
        aStorage.mPrograms.push_back({
            .mProgram = {
                graphics::makeLinkedProgram({
                    {GL_VERTEX_SHADER, gVertexShader},
                    {GL_FRAGMENT_SHADER, gFragmentShader},
                }),
                "white",
            },
        });

        Effect effect{
            .mTechniques{makeVector(
                Technique{
                    .mConfiguredProgram{ &aStorage.mPrograms.back() }
                }
            )},
        };
        aStorage.mEffects.push_back(std::move(effect));

        return Material{
            .mEffect = &aStorage.mEffects.back(),
        };
    }


    /// @brief Layout compatible with the shader's `ViewBlock`
    struct GpuViewBlock
    {
        GpuViewBlock(
            math::AffineMatrix<4, GLfloat> aWorldToCamera,
            math::Matrix<4, 4, GLfloat> aProjection 
        ) :
            mWorldToCamera{aWorldToCamera},
            mProjection{aProjection},
            mViewingProjection{aWorldToCamera * aProjection}
        {}

        GpuViewBlock(const Camera & aCamera) :
            GpuViewBlock{aCamera.getParentToCamera(), aCamera.getProjection()}
        {}

        math::AffineMatrix<4, GLfloat> mWorldToCamera; 
        math::Matrix<4, 4, GLfloat> mProjection; 
        math::Matrix<4, 4, GLfloat> mViewingProjection;
    };


    void loadFrameUbo(const graphics::UniformBufferObject & aUbo)
    {
        GLfloat time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
        graphics::loadSingle(aUbo, time, graphics::BufferHint::DynamicDraw);
    }


    void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
    {
        graphics::loadSingle(aUbo, GpuViewBlock{aCamera}, graphics::BufferHint::DynamicDraw);
    }


    void draw(const PassCache & aPassCache,
              const RepositoryUBO & aUboRepository,
              const RepositoryTexture & aTextureRepository)
    {
        //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
        // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
        // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
        // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
        // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

        GLuint firstInstance = 0; 
        for (const DrawCall & aCall : aPassCache.mCalls)
        {
            PROFILER_SCOPE_SECTION("drawcall_iteration", CpuTime);

            PROFILER_PUSH_SECTION("discard_2", CpuTime);
            const IntrospectProgram & selectedProgram = *aCall.mProgram;
            const graphics::VertexArrayObject & vao = *aCall.mVao;
            PROFILER_POP_SECTION;

            {
                PROFILER_PUSH_SECTION("bind_VAO", CpuTime);
                graphics::ScopedBind vaoScope{vao};
                PROFILER_POP_SECTION;
                
                {
                    PROFILER_SCOPE_SECTION("set_buffer_backed_blocks", CpuTime);
                    setBufferBackedBlocks(selectedProgram, aUboRepository);
                    // TODO #repos This should be consolidated
                    setBufferBackedBlocks(selectedProgram, aCall.mCallContext->mUboRepo);
                }

                {
                    PROFILER_SCOPE_SECTION("set_textures", CpuTime);
                    setTextures(selectedProgram, aTextureRepository);
                    // TODO #repos This should be consolidated
                    setTextures(selectedProgram, aCall.mCallContext->mTextureRepo);
                }

                PROFILER_PUSH_SECTION("bind_program", CpuTime);
                graphics::ScopedBind programScope{selectedProgram};
                PROFILER_POP_SECTION;

                {
                    // TODO Ad 2023/08/23: Measuring GPU time here has a x2 impact on cpu performance
                    // Can we have efficient GPU measures?
                    PROFILER_SCOPE_SECTION("glDraw_call", CpuTime/*, GpuTime*/);
                    
                    glMultiDrawElementsIndirect(
                        aCall.mPrimitiveMode,
                        aCall.mIndicesType,
                        (void *)(firstInstance * sizeof(DrawElementsIndirectCommand)),
                        aCall.mPartCount,
                        sizeof(DrawElementsIndirectCommand));
                }
            }
            firstInstance += aCall.mPartCount;
        }
    }

    void populatePartList(PartList & aPartList, const Node & aNode, const Pose & aParentPose)
    {
        const Pose & localPose = aNode.mInstance.mPose;
        Pose absolutePose = aParentPose.transform(localPose);

        if(Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            for(const Part & part: object->mParts)
            {
                const Material & material =
                    aNode.mInstance.mMaterialOverride ?
                    *aNode.mInstance.mMaterialOverride : part.mMaterial;

                aPartList.mParts.push_back(&part);
                aPartList.mMaterials.push_back(&material);
                // pushed after
                aPartList.mTransformIdx.push_back((GLsizei)aPartList.mInstanceTransforms.size());
            }

            aPartList.mInstanceTransforms.push_back(
                math::trans3d::scaleUniform(absolutePose.mUniformScale)
                * math::trans3d::translate(absolutePose.mPosition));
        }

        for(const auto & child : aNode.mChildren)
        {
            populatePartList(aPartList, child, absolutePose);
        }
    }


    PassCache preparePass(const PartList & aPartList,
                          Storage & aStorage)
    {
        PROFILER_SCOPE_SECTION("prepare_pass", CpuTime);

        PassCache result;
        // TODO Ad 2023/09/26: #multipass Handle several drawcalls, by sorting
        DrawCall singleCallAtm;

        if(!aPartList.mParts.empty())
        {
            const Part & part = *aPartList.mParts.front();
            const VertexStream & vertexStream = *part.mVertexStream;
            singleCallAtm.mPrimitiveMode = part.mPrimitiveMode;
            singleCallAtm.mIndicesType = vertexStream.mIndicesType;

            // TODO Implement program selection, this is a hardcoded quick test
            // (Ideally, a shader system with some form of render list)
            const Material & material = *aPartList.mMaterials.front();
            assert(material.mEffect->mTechniques.size() == 1);
            const ConfiguredProgram & configuredProgram = 
                *material.mEffect->mTechniques.at(0).mConfiguredProgram;
            const IntrospectProgram & selectedProgram = configuredProgram.mProgram;
            singleCallAtm.mProgram = &selectedProgram;

            singleCallAtm.mCallContext = part.mMaterial.mContext;

            // Note: the config is via "handle", hosted by in cache that is mutable, so loosing the constness is correct.
            auto vao =
                [&, &entries = configuredProgram.mConfig->mEntries]() 
                -> const graphics::VertexArrayObject *
                {
                    if(auto foundConfig = std::find_if(entries.begin(), entries.end(),
                                                    [&vertexStream, &part](const auto & aEntry)
                                                    {
                                                        return aEntry.mVertexStream == part.mVertexStream;
                                                    });
                        foundConfig != entries.end())
                    {
                        return foundConfig->mVao;
                    }
                    else
                    {
                        aStorage.mVaos.push_back(prepareVAO(selectedProgram, vertexStream));
                        entries.push_back(
                            ProgramConfig::Entry{
                                .mVertexStream = part.mVertexStream,
                                .mVao = &aStorage.mVaos.back(),
                            });
                        return entries.back().mVao;
                    }
                }();
            singleCallAtm.mVao = vao;

            // TODO Ad 2023/09/26: #multipass This should be changed.
            // Currently we expect all parts to be drawn by the same call.
            singleCallAtm.mPartCount = (GLsizei)aPartList.mParts.size();
        }

        for (std::size_t partIdx = 0; partIdx != aPartList.mParts.size(); ++partIdx)
        {
            const Part & part = *aPartList.mParts[partIdx];
            const VertexStream & vertexStream = *part.mVertexStream;
            // TODO #multipass
            assert(&vertexStream == aPartList.mParts.front()->mVertexStream);
            const Material & material = *aPartList.mMaterials[partIdx];

            // TODO #multipass handle segregation by those values, instead of requiring the same everywhere
            assert(part.mPrimitiveMode == singleCallAtm.mPrimitiveMode);
            assert(vertexStream.mIndicesType == singleCallAtm.mIndicesType);
            // Same effect implies same program would be selected 
            assert(part.mMaterial.mEffect == aPartList.mMaterials.front()->mEffect);
            assert(part.mMaterial.mContext == singleCallAtm.mCallContext);

            // TODO handle the case where there is an offset
            // This is more complicated with indirect draw commands, because they do not
            // accept a (void*) offset, but a number of indices (firstIndex).
            assert(vertexStream.mIndexBufferView.mOffset == 0);
            result.mDrawCommands.push_back(
                DrawElementsIndirectCommand{
                    .mCount = part.mIndicesCount,
                    // TODO Ad 2023/09/26: #bench Is it worth it to group identical parts and do "instanced" drawing?
                    // For the moment, draw a single instance for each part (i.e. no instancing)
                    .mInstanceCount = 1,
                    .mFirstIndex = (GLuint)(vertexStream.mIndexBufferView.mOffset 
                                            / graphics::getByteSize(vertexStream.mIndicesType))
                                    + part.mIndexFirst,
                    .mBaseVertex = part.mVertexFirst,
                    .mBaseInstance = (GLuint)partIdx,
                }
            );

            result.mDrawInstances.push_back(DrawInstance{
                .mInstanceTransformIdx = aPartList.mTransformIdx[partIdx],
                .mMaterialIdx = (GLsizei)material.mPhongMaterialIdx,
            });
        }

        result.mCalls.push_back(std::move(singleCallAtm));
        return result;
    }

} // unnamed namespace


PartList Scene::populatePartList() const
{
    PROFILER_SCOPE_SECTION("populate_draw_list", CpuTime);

    static constexpr Pose gIdentityPose{
        .mPosition{0.f, 0.f, 0.f},
        .mUniformScale = 1,
    };

    PartList partList;
    for(const Node & topNode : mRoot)
    {
        renderer::populatePartList(partList, topNode, gIdentityPose);
    }

    return partList;
}


HardcodedUbos::HardcodedUbos(Storage & aStorage)
{
    aStorage.mUbos.emplace_back();
    mFrameUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gFrame, mFrameUbo);

    aStorage.mUbos.emplace_back();
    mViewingUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gView, mViewingUbo);

    aStorage.mUbos.emplace_back();
    mModelTransformUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLocalToWorld, mModelTransformUbo);
}


void HardcodedUbos::addUbo(Storage & aStorage, BlockSemantic aSemantic, graphics::UniformBufferObject && aUbo)
{
    aStorage.mUbos.push_back(std::move(aUbo));
    mUboRepository.emplace(aSemantic, &aStorage.mUbos.back());
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


RenderGraph::RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                         const std::filesystem::path & aSceneFile,
                         const imguiui::ImguiUi & aImguiUi) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    mLoader{makeResourceFinder()},
    mUbos{mStorage},
    mCameraControl{mGlfwAppInterface->getWindowSize(),
                   gInitialVFov,
                   Orbital{2/*initial radius*/}
    }
{
    registerGlfwCallbacks(*mGlfwAppInterface, mCameraControl, EscKeyBehaviour::Close, &aImguiUi);
    registerGlfwCallbacks(*mGlfwAppInterface, mFirstPersonControl, EscKeyBehaviour::Close, &aImguiUi);

    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    // At the moment, hardcode a maximum number
    mInstanceStream = makeInstanceStream(mStorage, gMaxDrawInstances);

    //static Object triangle;
    //triangle.mParts.push_back(Part{
    //    .mVertexStream = makeTriangle(mStorage),
    //    .mMaterial = makeWhiteMaterial(mStorage),
    //});

    //mScene.addToRoot(Instance{
    //    .mObject = &triangle,
    //    .mPose = {
    //        .mPosition = {-0.5f, -0.2f, 0.f},
    //        .mUniformScale = 0.3f,
    //    }
    //});

    //// TODO cache materials
    //static Object cube;
    //cube.mParts.push_back(Part{
    //    .mVertexStream = makeCube(mStorage),
    //    .mMaterial = triangle.mParts[0].mMaterial,
    //});

    //mScene.addToRoot(Instance{
    //    .mObject = &cube,
    //    .mPose = {
    //        .mPosition = {0.5f, -0.2f, 0.f},
    //        .mUniformScale = 0.3f,
    //    }
    //});

    mScene = mLoader.loadScene(aSceneFile, "effects/Mesh.sefx", mInstanceStream, mStorage);
    /*const*/Node & model = mScene.mRoot.front();

    // TODO Ad 2023/10/03: Sort out this bit of logic: remove hardcoded sections,
    // better handle camera placement / projections scene wide
    // Setup instance and camera poses
    {
        // TODO automatically handle scaling via model bounding box.
        // Note that visually this has the same effect as moving the radial camera away
        // Actually, an automatic scaling could be detected for an "object viewer", but it is much harder for an environment
        // From the values of teapot and sponza, they seem to be in centimeters
        model.mInstance.mPose.mUniformScale = 0.01f;
        model.mAabb *= model.mInstance.mPose.mUniformScale;

        // Center the model on world origin
        model.mInstance.mPose.mPosition = -model.mAabb.center().as<math::Vec>();

        // move the orbital camera away, depending on the model size
        mCameraControl.mOrbital.mSpherical.radius() = 
            std::max(mCameraControl.mOrbital.mSpherical.radius(),
                    gRadialDistancingFactor * (*model.mAabb.mDimension.getMaxMagnitudeElement()));


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
}


void RenderGraph::update(float aDeltaTime)
{
    PROFILER_SCOPE_SECTION("RenderGraph::update()", CpuTime);
    mFirstPersonControl.update(aDeltaTime);
}


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
void RenderGraph::loadDrawBuffers(const PartList & aPartList,
                                  const PassCache & aPassCache)
{
    PROFILER_SCOPE_SECTION("load_draw_buffers", CpuTime);

    assert(aPassCache.mDrawInstances.size() <= gMaxDrawInstances);

    graphics::load(*mUbos.mModelTransformUbo,
                   std::span{aPartList.mInstanceTransforms},
                   graphics::BufferHint::DynamicDraw);

    graphics::load(*mInstanceStream.mVertexBufferViews.at(0).mGLBuffer,
                   std::span{aPassCache.mDrawInstances},
                   graphics::BufferHint::DynamicDraw);

    graphics::load(mIndirectBuffer,
                   std::span{aPassCache.mDrawCommands},
                   graphics::BufferHint::DynamicDraw);
}


void RenderGraph::render()
{
    PROFILER_SCOPE_SECTION("RenderGraph::render()", CpuTime, GpuTime);
    nvtx3::mark("RenderGraph::render()");
    NVTX3_FUNC_RANGE();

    // TODO: How to handle material/program selection while generating the part list,
    // if the camera (or pass itself?) might override the materials?
    PartList partList = mScene.populatePartList();

    // TODO should be done per pass
    PassCache passCache = preparePass(partList, mStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(partList, passCache);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO handle pipeline state with an abstraction
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // We implemented alpha testing (cut-out), no blending.
    glDisable(GL_BLEND);

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

    {
        PROFILER_SCOPE_SECTION("load_dynamic_UBOs", CpuTime, GpuTime);
        loadFrameUbo(*mUbos.mFrameUbo); // TODO Separate, the frame ubo should likely be at the top, once per frame
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, mCamera);
    }

    RepositoryTexture textureRepository;
    if(!mStorage.mTextures.empty())
    {
        // A single texture array at the moment
        assert(mStorage.mTextures.size() == 1);
        textureRepository = {{semantic::gDiffuseTexture, &mStorage.mTextures.front()}};
    }

    // Use the same indirect buffer for all drawings
    graphics::bind(mIndirectBuffer, graphics::BufferType::DrawIndirect);

    // TODO should be done once per viewport
    glViewport(0, 0,
               mGlfwAppInterface->getFramebufferSize().width(),
               mGlfwAppInterface->getFramebufferSize().height());

    {
        PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen);
        draw(passCache, mUbos.mUboRepository, textureRepository);
    }
}


} // namespace ad::renderer