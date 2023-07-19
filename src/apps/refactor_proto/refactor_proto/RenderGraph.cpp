#include "RenderGraph.h"

#include "Cube.h"
#include "Profiling.h"
#include "SetupDrawing.h"

#include <handy/vector_utils.h>

#include <math/Transformations.h>
#include <math/Vector.h>

#include <renderer/BufferLoad.h>
#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#include <array>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 410

    in vec3 v_Position_clip;

    layout(std140) uniform InstancePoseBlock
    {
        mat4 localToWorld;
    };

    void main()
    {
        gl_Position = localToWorld * vec4(v_Position_clip, 1.f);
    }
)#";

const char * gFragmentShader = R"#(
    #version 410

    out vec4 color;

    void main()
    {
        color = vec4(1.0, 0.5, 0.25, 1.f);
    }
)#";


namespace semantic
{
    const std::string gPosition{"Position"};
} // namespace semantic


namespace {

    template <class T_data, std::size_t N_extent>
    graphics::VertexBufferObject makeMutableBuffer(std::span<T_data, N_extent> aInitialData, GLenum aHint)
    {
        graphics::VertexBufferObject vbo; // glGenBuffers()
        graphics::ScopedBind boundVBO{vbo}; // glBind()
        glBufferData(
            vbo.GLTarget_v,
            aInitialData.size_bytes(),
            aInitialData.data(),
            aHint);
        return vbo;
    }

    VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions)
    {
        aStorage.mVertexBuffers.push_back(makeMutableBuffer(aPositions, GL_STATIC_DRAW));

        VertexBufferView vboView{
            .mGLBuffer = &aStorage.mVertexBuffers.back(),
            .mStride = sizeof(decltype(aPositions)::value_type),
            .mOffset = 0,
        };

        return VertexStream{
            .mBufferViews{ vboView, },
            .mSemanticToAttribute{
                {
                    semantic::gPosition,
                    AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = 3,
                            .mOffset = 0,
                            .mComponentType = GL_FLOAT
                        }
                    }
                },
            },
            .mVertexCount = (GLsizei)std::size(aPositions),
            .mPrimitiveMode = GL_TRIANGLES,
        };
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


    void draw(const Instance & aInstance)
    {
        RepositoryUBO ubos;
        {
            PROFILER_SCOPE_SECTION("prepare_UBO", CpuTime, GpuTime);

            graphics::UniformBufferObject ubo;
            math::AffineMatrix<4, GLfloat> modelTransformation = 
                math::trans3d::scaleUniform(aInstance.mPose.mUniformScale)
                * math::trans3d::translate(aInstance.mPose.mPosition)
                ;
            graphics::load(ubo,
                           std::span{&modelTransformation, 1}, 
                           graphics::BufferHint::StaticDraw/*todo change hint when refactoring*/);
            ubos.emplace("InstancePose", std::move(ubo));
        }

        for(const Part & part : aInstance.mObject->mParts)
        {
            // TODO replace program selection with something not hardcoded, this is a quick test
            // (Ideally, a shader system with some form of render list)
            assert(part.mMaterial.mEffect->mTechniques.size() == 1);
            const IntrospectProgram & selectedProgram = part.mMaterial.mEffect->mTechniques.at(0).mProgram;

            // TODO cache VAO
            PROFILER_BEGIN_SECTION("prepare_VAO", CpuTime);
            graphics::VertexArrayObject vao = prepareVAO(part.mVertexStream, selectedProgram);
            PROFILER_END_SECTION;
            graphics::ScopedBind vaoScope{vao};
            
            {
                PROFILER_SCOPE_SECTION("set_buffer_backed_blocks", CpuTime);
                setBufferBackedBlocks(ubos, selectedProgram);
            }

            graphics::ScopedBind programScope{selectedProgram};

            glDrawArrays(part.mVertexStream.mPrimitiveMode, 0, part.mVertexStream.mVertexCount);
        }
    }


} // unnamed namespace


RenderGraph::RenderGraph()
{
    // TODO replace use of pointers into the storage (which are invalidated on vector resize)
    // with some form of handle
    mStorage.mVertexBuffers.reserve(16);

    static Object triangle;
    triangle.mParts.push_back(Part{
        .mVertexStream = makeTriangle(mStorage),
        .mMaterial = makeWhiteMaterial(mStorage),
    });

    mScene.push_back(Instance{
        .mObject = &triangle,
        .mPose = {
            .mPosition = {-0.5f, 0.f, 0.f},
            .mUniformScale = 0.3f,
        }
    });

    // TODO cache materials
    static Object cube;
    cube.mParts.push_back(Part{
        .mVertexStream = makeCube(mStorage),
        .mMaterial = triangle.mParts[0].mMaterial,
    });

    mScene.push_back(Instance{
        .mObject = &cube,
        .mPose = {
            .mPosition = {0.5f, 0.f, 0.f},
            .mUniformScale = 0.3f,
        }
    });
}


void RenderGraph::render()
{
    PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen);
    for(const auto & instance : mScene)
    {
        // TODO replace with some form a render list generation, abstracting material/program selection
        draw(instance);
    }
}


} // namespace ad::renderer