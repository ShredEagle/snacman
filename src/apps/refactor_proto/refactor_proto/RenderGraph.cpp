#include "RenderGraph.h"

#include "Cube.h"
#include "Loader.h"
#include "Profiling.h"
#include "SetupDrawing.h"

#include <graphics/CameraUtilities.h>

#include <handy/vector_utils.h>

#include <math/Transformations.h>
#include <math/Vector.h>

#include <renderer/BufferLoad.h>
#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#include <array>
#include <fstream>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 420

    in vec3 v_Position_clip;

    in mat4 i_ModelTransform;

    layout(std140, binding = 1) uniform ViewBlock
    {
        mat4 viewingProjection;
    };

    void main()
    {
        gl_Position = viewingProjection * i_ModelTransform * vec4(v_Position_clip, 1.f);
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


namespace {

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
        Effect effect{
            .mTechniques{makeVector(
                Technique{
                    .mProgram{
                        graphics::makeLinkedProgram({
                            {GL_VERTEX_SHADER, gVertexShader},
                            {GL_FRAGMENT_SHADER, gFragmentShader},
                        }),
                        "white"
                    }
                }
            )},
        };
        aStorage.mEffects.push_back(std::move(effect));

        return Material{
            .mEffect = &aStorage.mEffects.back(),
        };
    }


    void draw(const Instance & aInstance,
              const SemanticBufferViews & aInstanceBufferView,
              math::Size<2, int> aFramebufferResolution)
    {
        // TODO should be done once per viewport
        glViewport(0, 0, aFramebufferResolution.width(), aFramebufferResolution.height());

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
                math::Box<GLfloat> viewVolume = 
                    graphics::getViewVolumeRightHanded(aFramebufferResolution,
                                                       2.f,
                                                       -0.1f,
                                                       10.f);
        
                math::AffineMatrix<4, GLfloat> viewingProjection = 
                    math::trans3d::translate<GLfloat>({0.f, 0.f, -2.f})
                    * math::trans3d::orthographicProjection(viewVolume);
                graphics::loadSingle(ubo, viewingProjection, 
                                     graphics::BufferHint::StaticDraw/*todo change hint when refactoring*/);
                ubos.emplace(semantic::gView, std::move(ubo));
            }
        }

        for(const Part & part : aInstance.mObject->mParts)
        {
            // TODO replace program selection with something not hardcoded, this is a quick test
            // (Ideally, a shader system with some form of render list)
            assert(part.mMaterial.mEffect->mTechniques.size() == 1);
            const IntrospectProgram & selectedProgram = part.mMaterial.mEffect->mTechniques.at(0).mProgram;

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


RenderGraph::RenderGraph()
{
    // TODO replace use of pointers into the storage (which are invalidated on vector resize)
    // with some form of handle
    mStorage.mBuffers.reserve(16);
    mStorage.mObjects.reserve(16);

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

    std::filesystem::path binaryFile{"D:/projects/Gamedev/2/proto-assets/teapot/teapot.seum"};
    Node teapot = loadBinary(binaryFile, mStorage, triangle.mParts[0].mMaterial);
    teapot.mInstance.mPose.mPosition = {0.0f, 0.2f, 0.f};
    teapot.mInstance.mPose.mUniformScale = 0.005f;
    mScene.mRoot.push_back(std::move(teapot));

    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    mInstanceStream = makeInstanceStream(mStorage, 1);
}


void RenderGraph::render(const graphics::ApplicationGlfw & aGlfwApp)
{
    // Implement material/program selection while generating drawlist
    DrawList drawList = mScene.populateDrawList();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen);
    for(const auto & instance : drawList)
    {
        draw(instance, mInstanceStream, aGlfwApp.getAppInterface()->getFramebufferSize());
    }
}


} // namespace ad::renderer