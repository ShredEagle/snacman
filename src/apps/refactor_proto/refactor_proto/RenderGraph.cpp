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


    graphics::UniformBufferObject prepareMaterialBuffer(const Storage & aStorage)
    {
        graphics::UniformBufferObject ubo;
        graphics::load(ubo, std::span{aStorage.mPhongMaterials}, graphics::BufferHint::StaticDraw);
        return ubo;
    }


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


    void draw(const Instance & aInstance,
              const GenericStream & aInstanceBufferView,
              Storage & aStorage,
              const RepositoryUBO & aUboRepository)
    {
        //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
        // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
        // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
        // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
        // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

        for(const Part & part : aInstance.mObject->mParts)
        {
            // TODO replace program selection with something not hardcoded, this is a quick test
            // (Ideally, a shader system with some form of render list)
            // Also, this selection should be done while the drawlist is generated, so it is shared by all passes
            // Also, the drawlist generation should be at the part level, not the object instance level
            const Material & material = aInstance.mMaterialOverride ?
                *aInstance.mMaterialOverride : part.mMaterial;
            
            // TODO should be done ahead of the draw call, at once for all instances.
            // TODO there is a tension between what is actually per object instance 
            // (the transformation matrix if there cannot be any per part transformation)
            // and what is per part (the material, ...). Note that the GL instance
            // corresponds to a Part in our data model.
            {
                PROFILER_SCOPE_SECTION("prepare_instance_transform", CpuTime);
                InstanceData instanceData{
                    .mModelTransform =
                        math::trans3d::scaleUniform(aInstance.mPose.mUniformScale)
                        * math::trans3d::translate(aInstance.mPose.mPosition),
                    .mMaterialIdx = (GLuint)material.mPhongMaterialIdx,
                };
                
                graphics::replaceSubset(
                    *aInstanceBufferView.mVertexBufferViews.at(0).mGLBuffer,
                    0, /*offset*/
                    std::span{&instanceData, 1}
                );
            }

            assert(material.mEffect->mTechniques.size() == 1);
            
            const ConfiguredProgram & configuredProgram = *material.mEffect->mTechniques.at(0).mConfiguredProgram;
            const IntrospectProgram & selectedProgram = configuredProgram.mProgram;

            const VertexStream & vertexStream = *part.mVertexStream;

            {
                PROFILER_BEGIN_SECTION("prepare_VAO", CpuTime);
                // Note: the config is via "handle", hosted by in cache that is mutable, so loosing the constness is correct.
                const auto & vao = [&, &entries = configuredProgram.mConfig->mEntries]() -> const graphics::VertexArrayObject &
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
                        entries.push_back(
                            ProgramConfig::Entry{
                                .mVertexStream = part.mVertexStream,
                                .mVao = prepareVAO(selectedProgram, vertexStream),
                            });
                        return entries.back().mVao;
                    }
                }();
                graphics::ScopedBind vaoScope{vao};
                PROFILER_END_SECTION;
                
                {
                    PROFILER_SCOPE_SECTION("set_buffer_backed_blocks", CpuTime);
                    setBufferBackedBlocks(selectedProgram, aUboRepository);
                }

                if(material.mPhongMaterialIdx != -1)
                {
                    if(auto textureIdx = aStorage.mPhongMaterials[material.mPhongMaterialIdx].mDiffuseMap.mTextureIndex;
                    textureIdx != TextureInput::gNoEntry)
                    {
                        PROFILER_SCOPE_SECTION("set_textures", CpuTime);
                        setTextures(selectedProgram, {{semantic::gDiffuseTexture, &aStorage.mTextures[textureIdx]},});
                    }
                }

                //PROFILER_BEGIN_SECTION("use_program", CpuTime, GpuTime);
                graphics::ScopedBind programScope{selectedProgram};
                //PROFILER_END_SECTION;

                {
                    PROFILER_SCOPE_SECTION("glDraw_call", CpuTime, GpuTime);
                    // TODO multi draw
                    if(vertexStream.mIndicesType == NULL)
                    {
                        glDrawArraysInstanced(
                            part.mPrimitiveMode,
                            part.mVertexFirst,
                            part.mVertexCount,
                            1);
                    }
                    else
                    {
                        glDrawElementsInstanced(
                            part.mPrimitiveMode,
                            part.mIndicesCount,
                            vertexStream.mIndicesType,
                            (const void *)
                                (vertexStream.mIndexBufferView.mOffset 
                                + (part.mIndexFirst * graphics::getByteSize(vertexStream.mIndicesType))),
                            1);
                    }
                }
            }
        }
    }

    void populateDrawList(DrawList & aDrawList, const Node & aNode, const Pose & aParentPose)
    {
        const Pose & localPose = aNode.mInstance.mPose;
        Pose absolutePose = aParentPose.transform(localPose);

        if(Object * object = aNode.mInstance.mObject;
           object != nullptr)
        {
            aDrawList.push_back(Instance{
                .mObject = object,
                .mPose = absolutePose,
                .mMaterialOverride = aNode.mInstance.mMaterialOverride,
            });
        }

        for(const auto & child : aNode.mChildren)
        {
            populateDrawList(aDrawList, child, absolutePose);
        }
    }

} // unnamed namespace


DrawList Scene::populateDrawList() const
{
    static constexpr Pose gIdentityPose{
        .mPosition{0.f, 0.f, 0.f},
        .mUniformScale = 1,
    };

    DrawList drawList;
    for(const Node & topNode : mRoot)
    {
        renderer::populateDrawList(drawList, topNode, gIdentityPose);
    }

    return drawList;
}


HardcodedUbos::HardcodedUbos(Storage & aStorage)
{
    aStorage.mUbos.emplace_back();
    mFrameUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gFrame, mFrameUbo);

    aStorage.mUbos.emplace_back();
    mViewingUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gView, mViewingUbo);
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
                         const std::filesystem::path & aModelFile,
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
    mInstanceStream = makeInstanceStream(mStorage, 1);

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

    Effect * phongEffect = mLoader.loadEffect("effects/Mesh.sefx", mStorage);
    static Material defaultPhongMaterial{
        .mEffect = phongEffect,
    };

    Node model = loadBinary(aModelFile, mStorage, defaultPhongMaterial, mInstanceStream);

    // TODO store and load names in the binary file format
    SELOG(info)("Loaded model '{}' with bouding box {}.", aModelFile.stem().string(), fmt::streamed(model.mAabb));

    mUbos.addUbo(mStorage, semantic::gMaterials, prepareMaterialBuffer(mStorage));

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

    // TODO restore the ability to make a model copy, with a distinct material
    // Add a a copy of the model (hardcoded for teapot), to test another material idx
    //{
    //    Node copy = model;
    //    Material materialOverride{
    //        .mPhongMaterialIdx = mStorage.mPhongMaterials.size(), // The index of the material that will be pushed.
    //        .mEffect = phongEffect,
    //    };
    //    copy.mInstance.mPose.mPosition.y() = -0.8f;
    //    copy.mChildren.at(0).mInstance.mMaterialOverride = materialOverride;
    //    copy.mChildren.at(1).mInstance.mMaterialOverride = materialOverride;
    //    mScene.mRoot.push_back(std::move(copy));
    //}

    //// Creates another phong material, for the model copy
    //{
    //    mStorage.mPhongMaterials.push_back(mStorage.mPhongMaterials.at(0));
    //    auto & phong = mStorage.mPhongMaterials.at(1);
    //    phong.mDiffuseColor = math::hdr::gCyan<float> * 0.75f;
    //    phong.mAmbientColor = math::hdr::gCyan<float> * 0.2f;
    //    phong.mSpecularColor = math::hdr::gWhite<float> * 0.5f;
    //    phong.mSpecularExponent = 100.f;
    //}

    mScene.mRoot.push_back(std::move(model));

}


void RenderGraph::update(float aDeltaTime)
{
    mFirstPersonControl.update(aDeltaTime);
}


void RenderGraph::render()
{
    // Implement material/program selection while generating drawlist
    DrawList drawList = mScene.populateDrawList();

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
        loadFrameUbo(*mUbos.mFrameUbo);
        // Note in a more realistic application, several cameras would be used per frame.
        loadCameraUbo(*mUbos.mViewingUbo, mCamera);
    }

    PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen);
    // TODO should be done once per viewport
    glViewport(0, 0,
               mGlfwAppInterface->getFramebufferSize().width(),
               mGlfwAppInterface->getFramebufferSize().height());
    for(const auto & instance : drawList)
    {
        draw(instance,
             mInstanceStream,
             mStorage,
             mUbos.mUboRepository);
    }
}


} // namespace ad::renderer