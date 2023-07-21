#include "RenderGraph.h"

#include "Cube.h"
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


struct InstanceData
{
    math::AffineMatrix<4, GLfloat> mModelTransform;
};


namespace semantic
{
    const Semantic gPosition{"Position"};
    const Semantic gModelTransform{"ModelTransform"};

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
} // namespace semantic


namespace {

    template <class T_data>
    std::pair<graphics::BufferAny, GLsizeiptr>
    makeMutableBuffer(std::size_t aInstanceCount, GLenum aHint)
    {
        graphics::BufferAny buffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{buffer, target}; // glBind()
        const GLsizeiptr size = sizeof(T_data) * aInstanceCount;
        glBufferData(
            static_cast<GLenum>(target),
            size,
            nullptr,
            aHint);
        return std::make_pair(std::move(buffer), size);
    }


    template <class T_data, std::size_t N_extent>
    std::pair<graphics::BufferAny, GLsizeiptr>
    makeMutableBuffer(std::span<T_data, N_extent> aInitialData, GLenum aHint)
    {
        graphics::BufferAny buffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{buffer, target}; // glBind()
        const GLsizeiptr size = aInitialData.size_bytes();
        // TODO enable GL 4.5+, here used for DSA
        glBufferData(
            static_cast<GLenum>(target),
            size,
            aInitialData.data(),
            aHint);
        return std::make_pair(std::move(buffer), size);
    }

    VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions)
    {
        auto [vbo, size] = makeMutableBuffer(aPositions, GL_STATIC_DRAW);
        aStorage.mBuffers.push_back(std::move(vbo));

        BufferView vboView{
            .mGLBuffer = &aStorage.mBuffers.back(),
            .mStride = sizeof(decltype(aPositions)::value_type),
            .mOffset = 0,
            .mSize = size, // The view has access to the whole buffer
        };

        return VertexStream{
            .mVertexBufferViews{ vboView, },
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


    // IMPORTANT: for the moment, just load the first vertex stream in the file
    // has to be extended to actually load complex models.
    VertexStream loadBinary(Storage & aStorage, std::istream & aBinaryStream)
    {
        // TODO we need a unified design where the load code does not duplicate the type of the write.
        unsigned int verticesCount;
        aBinaryStream.read((char *)&verticesCount, sizeof(verticesCount));

        constexpr auto gPositionSize = 3 * sizeof(float); // TODO that is crazy coupling
        std::size_t positionBufferSize = verticesCount * gPositionSize;
        std::unique_ptr<char[]> positionBuffer{new char[positionBufferSize]};
        aBinaryStream.read(positionBuffer.get(), positionBufferSize);

        auto [vbo, sizeVbo] = 
            makeMutableBuffer(std::span{positionBuffer.get(), positionBufferSize}, GL_STATIC_DRAW);

        aStorage.mBuffers.push_back(std::move(vbo));

        BufferView vboView{
            .mGLBuffer = &aStorage.mBuffers.back(),
            .mStride = gPositionSize,
            .mOffset = 0,
            .mSize = sizeVbo, // The view has access to the whole buffer
        };

        unsigned int primitiveCount;
        aBinaryStream.read((char *)&primitiveCount, sizeof(primitiveCount));
        const unsigned int indicesCount = 3 * primitiveCount;

        constexpr auto gIndexSize = 3 * sizeof(unsigned int);
        const std::size_t indexBufferSize = indicesCount * gIndexSize;
        std::unique_ptr<char[]> indexBuffer{new char[indexBufferSize]};
        aBinaryStream.read(indexBuffer.get(), indexBufferSize);

        auto [ibo, sizeIbo] = 
            makeMutableBuffer(std::span{indexBuffer.get(), indexBufferSize}, GL_STATIC_DRAW);

        aStorage.mBuffers.push_back(std::move(ibo));

        BufferView iboView{
            .mGLBuffer = &aStorage.mBuffers.back(),
            .mOffset = 0,
            .mSize = sizeIbo, // The view has access to the whole buffer
        };

        return VertexStream{
            .mVertexBufferViews{ vboView, },
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
            .mVertexCount = (GLsizei)verticesCount,
            .mPrimitiveMode = GL_TRIANGLES,
            .mIndexBufferView = iboView,
            .mIndicesType = GL_UNSIGNED_INT,
            .mIndicesCount = (GLsizei)indicesCount,
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

    SemanticBufferViews makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount)
    {
        auto [vbo, size] = makeMutableBuffer<InstanceData>(aInstanceCount, GL_STREAM_DRAW);
        aStorage.mBuffers.push_back(std::move(vbo));

        BufferView vboView{
            .mGLBuffer = &aStorage.mBuffers.back(),
            .mStride = sizeof(InstanceData),
            .mOffset = 0,
            .mSize = size, // The view has access to the whole buffer
        };

        return SemanticBufferViews{
            .mVertexBufferViews = { vboView, },
            .mSemanticToAttribute{
                {
                    semantic::gModelTransform,
                    AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = {4, 4},
                            .mOffset = 0,
                            .mComponentType = GL_FLOAT
                        },
                        .mInstanceDivisor = 1,
                    }
                },
            }
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


} // unnamed namespace


RenderGraph::RenderGraph()
{
    // TODO replace use of pointers into the storage (which are invalidated on vector resize)
    // with some form of handle
    mStorage.mBuffers.reserve(16);

    static Object triangle;
    triangle.mParts.push_back(Part{
        .mVertexStream = makeTriangle(mStorage),
        .mMaterial = makeWhiteMaterial(mStorage),
    });

    mScene.push_back(Instance{
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

    mScene.push_back(Instance{
        .mObject = &cube,
        .mPose = {
            .mPosition = {0.5f, -0.2f, 0.f},
            .mUniformScale = 0.3f,
        }
    });

    static Object teapot;
    std::ifstream teapotStream{"D:/projects/Gamedev/2/proto-assets/teapot/teapot.seum", std::ios::binary};
    teapot.mParts.push_back(Part{
        .mVertexStream = loadBinary(mStorage, teapotStream),
        .mMaterial = triangle.mParts[0].mMaterial,
    });

    mScene.push_back(Instance{
        .mObject = &teapot,
        .mPose = {
            .mPosition = {0.0f, 0.2f, 0.f},
            .mUniformScale = 0.005f,
        }
    });

    // TODO How do we handle the dynamic nature of the number of instance that might be renderered?
    mInstanceStream = makeInstanceStream(mStorage, 1);
}


void RenderGraph::render(const graphics::ApplicationGlfw & aGlfwApp)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    PROFILER_SCOPE_SECTION("draw_instances", CpuTime, GpuTime, GpuPrimitiveGen);
    for(const auto & instance : mScene)
    {
        // TODO replace with some form a render list generation, abstracting material/program selection
        draw(instance, mInstanceStream, aGlfwApp.getAppInterface()->getFramebufferSize());
    }
}


} // namespace ad::renderer