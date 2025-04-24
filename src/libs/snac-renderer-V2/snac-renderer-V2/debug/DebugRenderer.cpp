#include "DebugRenderer.h"

#include "../Cube.h"
#include "../Pass.h"
#include "../RendererReimplement.h"
#include "../Semantics.h"
#include "../SetupDrawing.h"
#include "../utilities/VertexStreamUtilities.h"

#include "../files/Loader.h"

// TODO #pose remove once we use poses as Entry Instance data
#include <math/Transformations.h>

namespace ad::renderer {


namespace {
    using LineVertex = snac::DebugDrawer::LineVertex;

    static const std::array<InterleavedAttributeDescription, 2> gLineVertexAttributes {{
        {
            AttributeDescription{
                .mSemantic = semantic::gPosition,
                .mDimension = 3,
                .mComponentType = GL_FLOAT,
            },
            offsetof(LineVertex, mPosition),
        },
        {
            AttributeDescription{
                .mSemantic = semantic::gColor,
                .mDimension = 4,
                .mComponentType = GL_FLOAT,
            },
            offsetof(LineVertex, mColor),
        },
    }};


    struct EntryVertex 
    {
        math::Position<3, float> mPosition;
    };


    static const std::array<InterleavedAttributeDescription, 1> gEntryVertexAttributes {{
        {
            AttributeDescription{
                .mSemantic = semantic::gPosition,
                .mDimension = 3,
                .mComponentType = GL_FLOAT,
            },
            offsetof(LineVertex, mPosition),
        },
    }};


    // TODO Ad 2024/04/16: #pose We would  ideally use the DebugDrawer::Entry directly as instance data
    // (but it mean we would either have a compute path to issue the transform matrices, or use poses directly)
    struct EntryInstance
    {
        math::Matrix<4, 4, float> mModelTransform;
        math::hdr::Rgba_f mColor;
    };


    static const std::array<InterleavedAttributeDescription, 2> gInstanceAttributes {{
        {
            AttributeDescription{
                .mSemantic = semantic::gModelTransform,
                .mDimension = {4, 4},
                .mComponentType = GL_FLOAT,
            },
            offsetof(EntryInstance, mModelTransform),
        },
        {
            AttributeDescription{
                .mSemantic = semantic::gColor,
                .mDimension = 4,
                .mComponentType = GL_FLOAT,
            },
            offsetof(EntryInstance, mColor),
        },
    }};


    template <class T_maker>
    std::vector<EntryVertex> prepareShapeVertices()
    {
        std::vector<EntryVertex> result;
        result.reserve(T_maker::gVertexCount);

        for(unsigned int idx = 0; idx != T_maker::gVertexCount; ++idx)
        {
            result.push_back(EntryVertex{
                .mPosition = T_maker::getPosition(idx),
            });
        }

        return result;
    }

    // To allow specialization of prepareShapeVertices
    struct BoxTag
    {
        static constexpr GLenum gPrimitiveMode = cube::Maker::gPrimitiveMode;
    };


    // TODO Ad 2024/04/17: #shapes This is symptomatic of now having a good design for basic shapes
    // Since we do not want the default cube [-1, 1]^3, we made a specialization of 
    // prepareShapeVertices for the box [0, 1]^3.
    // This mostly repeats the base implementation...
    template <>
    std::vector<EntryVertex> prepareShapeVertices<BoxTag>()
    {
        std::vector<EntryVertex> result;
        result.reserve(cube::Maker::gVertexCount);
        constexpr math::Box<float> box{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}};

        for(unsigned int idx = 0; idx != cube::Maker::gVertexCount; ++idx)
        {
            result.push_back(EntryVertex{
                .mPosition = cube::Maker::getPosition(idx, box),
            });
        }

        return result;
    }

    Handle<VertexStream> setupVertexStream(Storage & aStorage,
                                           const std::span<const InterleavedAttributeDescription> aAttributes,
                                           GLuint aInstanceDivisor,
                                           GLsizei aStride,
                                           Handle<const graphics::BufferAny> aBuffer)
    {
        Handle<VertexStream> result = primeVertexStream(aStorage);
        addInterleavedAttributes(
            result,
            aStride,
            aAttributes,
            aBuffer,
            0, // No vertices on construction : the buffer will be loaded on rendering
            aInstanceDivisor
        );
        return result;
    }


    template <class T_maker>
    Part setupInstancedShape(std::string aName, 
                             Storage & aStorage,
                             Handle<const graphics::BufferAny> aInstanceBuffer)
    {
        Handle<VertexStream> vertexStream = setupVertexStream(aStorage,
                                                              gInstanceAttributes, 
                                                              1, // Instance attribute
                                                              sizeof(EntryInstance),
                                                              aInstanceBuffer);

        auto vertices = prepareShapeVertices<T_maker>();

        // Both call are partially redundant, the API should be better.
        Handle<graphics::BufferAny> vertexBuffer = 
            makeBuffer(sizeof(EntryVertex), vertices.size(), GL_STATIC_DRAW, aStorage);
        proto::load(*vertexBuffer, std::span{vertices}, graphics::BufferHint::StaticDraw);

        addInterleavedAttributes(
            vertexStream,
            sizeof(EntryVertex),
            gEntryVertexAttributes,
            vertexBuffer,
            (unsigned int)vertices.size(),
            0 // Vertex attribute
        );

        return Part{
            .mName = std::move(aName),
            .mVertexStream = std::move(vertexStream),
            .mPrimitiveMode = T_maker::gPrimitiveMode,
            .mVertexCount = (GLuint)vertices.size(),
        };
    }

} // unnamed namespace


DebugRenderer::DebugRenderer(Storage & aStorage, const Loader & aLoader) :
    mLineVertexBuffer{makeBuffer(0, 0, GL_STREAM_DRAW, aStorage)},
    mLines{
        .mName = "DebugLines",
        .mVertexStream = setupVertexStream(aStorage,
                                           gLineVertexAttributes, 
                                           0, // Vertex attribute
                                           sizeof(snac::DebugDrawer::LineVertex),
                                           mLineVertexBuffer),
        .mPrimitiveMode = GL_LINES,
    },
    mLineProgram{storeConfiguredProgram(aLoader.loadProgram("programs/DebugDraw.prog"), aStorage)},
    mEntryInstanceBuffer{makeBuffer(0, 0, GL_STREAM_DRAW, aStorage)},
    mEntryProgram{storeConfiguredProgram(aLoader.loadProgram("programs/DebugDrawModelTransform.prog"), aStorage)},
    mBox{setupInstancedShape<BoxTag>("DebugBox", aStorage, mEntryInstanceBuffer)},
    mArrow{setupInstancedShape<arrow::Maker>("DebugArrow", aStorage, mEntryInstanceBuffer)}
{

}

void DebugRenderer::render(const snac::DebugDrawer::DrawList & aDrawList,
                           const RepositoryUbo & aUboRepository,
                           Storage & aStorage)
{
    gl.Disable(GL_DEPTH_TEST);
    gl.Disable(GL_CULL_FACE);
    gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    drawLines(aDrawList.mCommands->mLineVertices, aUboRepository, aStorage);
    drawPosedEntry({
                        {&mBox, aDrawList.mCommands->mBoxes}, 
                        {&mArrow, aDrawList.mCommands->mArrows},
                   },
                   aUboRepository, 
                   aStorage);

    gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl.Enable(GL_CULL_FACE);
    gl.Enable(GL_DEPTH_TEST);
}


void DebugRenderer::drawLines(std::span<snac::DebugDrawer::LineVertex> aLines,
                              const RepositoryUbo & aUboRepository,
                              Storage & aStorage)
{
    proto::load(*mLineVertexBuffer, aLines, graphics::BufferHint::StreamDraw);

    // The VAO might actually be cached as a data member, since it will be constant during runtime
    Handle<graphics::VertexArrayObject> vao = getVao(*mLineProgram, mLines, aStorage);
    graphics::ScopedBind vaoScope{*vao};
    graphics::ScopedBind programScope{mLineProgram->mProgram};

    // TODO this looks like something that could be done only once (will not change program side)
    // or at least optimized (the binding points of the UBO might be changed client side, by other code)
    setBufferBackedBlocks(mLineProgram->mProgram, aUboRepository);

    gl.DrawArrays(mLines.mPrimitiveMode, 0, (GLsizei)aLines.size());
}


void DebugRenderer::drawPosedEntry(const std::vector<std::pair<Part *, std::span<snac::DebugDrawer::Entry>>> & aEntries,
                                   const RepositoryUbo & aUboRepository,
                                   Storage & aStorage)
{
    // TODO should be kept between frames, no need to reallocate each time...
    // on the other hand, when we will push poses directly, we will not need this intermediary storage anymore
    std::vector<EntryInstance> instances;
    for (const auto & [_part, entries] : aEntries)
    {
        for (const auto & entry : entries)
        {
            instances.push_back(EntryInstance{
                .mModelTransform = math::trans3d::scale(entry.mScaling)
                                    * entry.mOrientation.toRotationMatrix()
                                    * math::trans3d::translate(
                                        entry.mPosition.as<math::Vec>()),
                .mColor = entry.mColor,
            });
        }
    }
    proto::load(*mEntryInstanceBuffer, std::span{instances}, graphics::BufferHint::StreamDraw);

    graphics::ScopedBind programScope{mEntryProgram->mProgram};

    GLuint baseInstance = 0;
    for (const auto & [part, entries] : aEntries)
    {
        // The VAO might actually be cached as a data member, since it will be constant during runtime
        Handle<graphics::VertexArrayObject> vao = getVao(*mEntryProgram, *part, aStorage);
        graphics::ScopedBind vaoScope{*vao};

        // TODO this looks like something that could be done only once (will not change program side)
        // or at least optimized (the binding points of the UBO might be changed client side, by other code)
        setBufferBackedBlocks(mEntryProgram->mProgram, aUboRepository);

        gl.DrawArraysInstancedBaseInstance(part->mPrimitiveMode,
                                           0,
                                           part->mVertexCount,
                                           (GLsizei)entries.size(),
                                           baseInstance);
        baseInstance += (GLuint)entries.size();
    }
}


} // namespace ad::renderer