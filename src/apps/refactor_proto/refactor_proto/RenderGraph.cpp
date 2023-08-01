#include "RenderGraph.h"

#include "Cube.h"
#include "Json.h"
#include "Loader.h"
#include "Profiling.h"
#include "SetupDrawing.h"

#include <handy/vector_utils.h>

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
constexpr float gFarZ{-25.f};

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



    VertexStream makeTriangle(Storage & aStorage)
    {
        std::array<math::Position<3, GLfloat>, 3> vertices{{
            { 1.f, -1.f, 0.f},
            { 0.f,  1.f, 0.f},
            {-1.f, -1.f, 0.f},
        }};
        return makeFromPositions(aStorage, std::span{vertices});
    }

    VertexStream makeCube(Storage & aStorage)
    {
        auto cubeVertices = getExpandedCubeVertices<math::Position<3, GLfloat>>();
        return makeFromPositions(aStorage, std::span{cubeVertices});
    }

    Material makeWhiteMaterial(Storage & aStorage)
    {
        aStorage.mPrograms.emplace_back(
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, gVertexShader},
                {GL_FRAGMENT_SHADER, gFragmentShader},
            }),
            "white");

        Effect effect{
            .mTechniques{makeVector(
                Technique{
                    .mProgram{ &aStorage.mPrograms.back() }
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



    void draw(const Instance & aInstance,
              const SemanticBufferViews & aInstanceBufferView,
              const Camera & aCamera,
              const Storage & aStorage)
    {
        // TODO should be done ahead of the draw call, at once for all instances.
        {
            math::AffineMatrix<4, GLfloat> modelTransformation = 
                math::trans3d::scaleUniform(aInstance.mPose.mUniformScale)
                * math::trans3d::translate(aInstance.mPose.mPosition)
                ;
            
            graphics::replaceSubset(
                *aInstanceBufferView.mVertexBufferViews.at(0).mGLBuffer,
                0, /*offset*/
                std::span{&modelTransformation, 1}
            );
        }

        //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
        // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
        // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
        // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
        // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

        // TODO Consolidate UBO setup
        RepositoryUBO ubos;
        {
            PROFILER_SCOPE_SECTION("prepare_UBO", CpuTime, GpuTime);
            {
                graphics::UniformBufferObject ubo;
                GLfloat time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
                graphics::loadSingle(ubo, time, 
                                     graphics::BufferHint::StaticDraw/*todo change hint when refactoring*/);
                ubos.emplace(semantic::gFrame, std::move(ubo));
            }
            {
                graphics::UniformBufferObject ubo;
                    
                graphics::loadSingle(ubo, 
                                     GpuViewBlock{aCamera}, 
                                     graphics::BufferHint::StaticDraw/*todo change hint when refactoring*/);
                ubos.emplace(semantic::gView, std::move(ubo));
            }
        }

        for(const Part & part : aInstance.mObject->mParts)
        {
            // TODO replace program selection with something not hardcoded, this is a quick test
            // (Ideally, a shader system with some form of render list)
            const Material & material = part.mMaterial;
            assert(material.mEffect->mTechniques.size() == 1);
            const IntrospectProgram & selectedProgram = *material.mEffect->mTechniques.at(0).mProgram;

            const VertexStream & vertexStream = part.mVertexStream;

            // TODO cache VAO
            PROFILER_BEGIN_SECTION("prepare_VAO", CpuTime);
            graphics::VertexArrayObject vao = prepareVAO(selectedProgram, vertexStream, {&aInstanceBufferView});
            PROFILER_END_SECTION;
            graphics::ScopedBind vaoScope{vao};
            
            {
                PROFILER_SCOPE_SECTION("set_buffer_backed_blocks", CpuTime);
                setBufferBackedBlocks(selectedProgram, ubos);
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

            graphics::ScopedBind programScope{selectedProgram};

            // TODO multi draw
            if(vertexStream.mIndicesType == NULL)
            {
                glDrawArraysInstanced(
                    vertexStream.mPrimitiveMode,
                    0,
                    vertexStream.mVertexCount,
                    1);
            }
            else
            {
                glDrawElementsInstanced(
                    vertexStream.mPrimitiveMode,
                    vertexStream.mIndicesCount,
                    vertexStream.mIndicesType,
                    (const void *)vertexStream.mIndexBufferView.mOffset,
                    1);
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


//TODO Ad 2023/07/26: Move to the glfw app library, handle all callbacks 
//    (and only register the subset actually provided by T_callbackProvider)
template <class T_callbackProvider>
void registerGlfwCallbacks(graphics::AppInterface & aAppInterface, T_callbackProvider & aProvider)
{
    using namespace std::placeholders;
    aAppInterface.registerMouseButtonCallback(
        std::bind(&T_callbackProvider::callbackMouseButton, std::ref(aProvider), _1, _2, _3, _4, _5));
    aAppInterface.registerCursorPositionCallback(
        std::bind(&T_callbackProvider::callbackCursorPosition, std::ref(aProvider), _1, _2));
    aAppInterface.registerScrollCallback(
        std::bind(&T_callbackProvider::callbackScroll, std::ref(aProvider), _1, _2));
}


RenderGraph::RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                         const std::filesystem::path & aModelFile) :
    mGlfwAppInterface{std::move(aGlfwAppInterface)},
    mLoader{makeResourceFinder()},
    mCameraControl{mGlfwAppInterface->getWindowSize(),
                   gInitialVFov,
                   Orbital{2/*initial radius*/}
    }
{
    mCamera.setupOrthographicProjection({
        .mAspectRatio = math::getRatio<GLfloat>(mGlfwAppInterface->getWindowSize()),
        .mViewHeight = mCameraControl.getViewHeightAtOrbitalCenter(),
        .mNearZ = gNearZ,
        .mFarZ = gFarZ,
    });

    registerGlfwCallbacks(*mGlfwAppInterface, mCameraControl);

    // TODO replace use of pointers into the storage (which are invalidated on vector resize)
    // with some form of handle
    mStorage.mBuffers.reserve(16);
    mStorage.mObjects.reserve(16);
    mStorage.mEffects.reserve(16);
    mStorage.mPrograms.reserve(16);
    mStorage.mPhongMaterials.reserve(16);
    mStorage.mTextures.reserve(16);

    static Object triangle;
    triangle.mParts.push_back(Part{
        .mVertexStream = makeTriangle(mStorage),
        .mMaterial = makeWhiteMaterial(mStorage),
    });

    mScene.addToRoot(Instance{
        .mObject = &triangle,
        .mPose = {
            .mPosition = {-0.5f, -0.2f, 0.f},
            .mUniformScale = 0.3f,
        }
    });

    // TODO cache materials
    static Object cube;
    cube.mParts.push_back(Part{
        .mVertexStream = makeCube(mStorage),
        .mMaterial = triangle.mParts[0].mMaterial,
    });

    mScene.addToRoot(Instance{
        .mObject = &cube,
        .mPose = {
            .mPosition = {0.5f, -0.2f, 0.f},
            .mUniformScale = 0.3f,
        }
    });

    static Material defaultPhongMaterial{
        .mEffect = mLoader.loadEffect("effects/Mesh.sefx", mStorage, {}),
    };
    Node model = loadBinary(aModelFile, mStorage, defaultPhongMaterial);
    model.mInstance.mPose.mPosition = {0.0f, 0.2f, 0.f};
    // TODO automatically handle scaling via model bounding box.
    model.mInstance.mPose.mUniformScale = 0.005f;
    mScene.mRoot.push_back(std::move(model));

    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    mInstanceStream = makeInstanceStream(mStorage, 1);
}


void RenderGraph::render()
{
    // Implement material/program selection while generating drawlist
    DrawList drawList = mScene.populateDrawList();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO handle pipeline state with an abstraction
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Update camera to match current values in orbital control.
    mCamera.setPose(mCameraControl.mOrbital.getParentToLocal());
    if(mCamera.isProjectionOrthographic())
    {
        // Note: to allow "zooming" in the orthographic case, we change the viewed height of the ortho projection.
        // An alternative would be to apply a scale factor to the camera Pose transformation.
        changeOrthographicViewportHeight(mCamera, mCameraControl.getViewHeightAtOrbitalCenter());
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
             mCamera,
             mStorage);
    }
}


} // namespace ad::renderer