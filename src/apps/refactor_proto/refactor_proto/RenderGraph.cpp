#include "RenderGraph.h"

#include "Profiling.h"
#include "SetupVertexAttributes.h"

#include <handy/vector_utils.h>

#include <math/Vector.h>

#include <renderer/Query.h>
#include <renderer/ScopeGuards.h>

#include <array>


namespace ad::renderer {


const char * gVertexShader = R"#(
    #version 410

    in vec3 v_Position_clip;

    void main()
    {
        gl_Position = vec4(v_Position_clip, 1.f);
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


    VertexStream makeTriangle(Storage & aStorage)
    {
        std::array<math::Position<3, GLfloat>, 3> vertices{{
            { 1.f, -1.f, 0.f},
            { 0.f,  1.f, 0.f},
            {-1.f, -1.f, 0.f},
        }};
        aStorage.mVertexBuffers.push_back(makeMutableBuffer(std::span{vertices}, GL_STATIC_DRAW));

        VertexBufferView vboView{
            .mGLBuffer = &aStorage.mVertexBuffers.back(),
            .mStride = sizeof(decltype(vertices)::value_type),
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
            .mVertexCount = (GLsizei)std::size(vertices),
            .mPrimitiveMode = GL_TRIANGLES,
        };
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
        for(const Part & part : aInstance.mObject->mParts)
        {
            // TODO replace program selection with something not hardcoded, this is a quick test
            // (Ideally, a shader system with some form a render list)
            assert(part.mMaterial.mEffect->mTechniques.size() == 1);
            const IntrospectProgram & selectedProgram = part.mMaterial.mEffect->mTechniques.at(0).mProgram;

            // TODO cache VAO
            graphics::VertexArrayObject vao = prepareVAO(part.mVertexStream, selectedProgram);
            graphics::ScopedBind vaoScope{vao};

            graphics::ScopedBind programScope{selectedProgram};

            glDrawArrays(part.mVertexStream.mPrimitiveMode, 0, part.mVertexStream.mVertexCount);
        }
    }


} // unnamed namespace


RenderGraph::RenderGraph()
{
    static Object triangle;
    triangle.mParts.push_back(Part{
        .mVertexStream = makeTriangle(mStorage),
        .mMaterial = makeWhiteMaterial(mStorage),
    });

    mScene.push_back(Instance{
        .mObject = &triangle,
        //.mPose
    });
}


void RenderGraph::render()
{
    PROFILER_BEGIN_SECTION("draw_instances");
    for(const auto & instance : mScene)
    {
        // TODO replace with some form a render list generation, abstracting material/program selection
        draw(instance);
    }
    PROFILER_END_SECTION;
}


} // namespace ad::renderer