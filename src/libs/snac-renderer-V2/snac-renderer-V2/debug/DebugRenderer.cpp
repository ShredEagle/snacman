#include "DebugRenderer.h"

#include "../Cube.h"
#include "../Pass.h"
#include "../RendererReimplement.h"
#include "../Semantics.h"
#include "../SetupDrawing.h"
#include "../VertexStreamUtilities.h"

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


    template <class T_Maker>
    std::vector<EntryVertex> prepareShapeVertices()
    {
        std::vector<EntryVertex> result;
        result.reserve(T_Maker::gVertexCount);

        for(unsigned int idx = 0; idx != T_Maker::gVertexCount; ++idx)
        {
            result.push_back(EntryVertex{
                .mPosition = T_Maker::getPosition(idx),
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


    Part setupBox(Storage & aStorage,
                  Handle<const graphics::BufferAny> aInstanceBuffer)
    {
        Handle<VertexStream> result = setupVertexStream(aStorage,
                                                        gInstanceAttributes, 
                                                        1, // Instance attribute
                                                        sizeof(snac::DebugDrawer::Entry),
                                                        aInstanceBuffer);

        auto boxVertices = prepareShapeVertices<cube::Maker>();

        // Both call are partially redundant, the API should be better.
        Handle<graphics::BufferAny> vertexBuffer = 
            makeBuffer(sizeof(EntryVertex), boxVertices.size(), GL_STATIC_DRAW, aStorage);
        proto::load(*vertexBuffer, std::span{boxVertices}, graphics::BufferHint::StaticDraw);

        addInterleavedAttributes(
            result,
            sizeof(EntryVertex),
            gEntryVertexAttributes,
            vertexBuffer,
            (unsigned int)boxVertices.size(),
            0 // Vertex attribute
        );

        return Part{
            .mName = "DebugBox",
            .mVertexStream = std::move(result),
            .mPrimitiveMode = GL_TRIANGLES,
            .mVertexCount = (GLuint)boxVertices.size(),
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
    mLineProgram{storeConfiguredProgram(aLoader.loadProgram("shaders/DebugDraw.prog"), aStorage)},
    mEntryInstanceBuffer{makeBuffer(0, 0, GL_STREAM_DRAW, aStorage)},
    mBox{setupBox(aStorage, mEntryInstanceBuffer)},
    mEntryProgram{storeConfiguredProgram(aLoader.loadProgram("shaders/DebugDrawModelTransform.prog"), aStorage)}
{

}

void DebugRenderer::render(snac::DebugDrawer::DrawList aDrawList,
                           const RepositoryUbo & aUboRepository,
                           Storage & aStorage)
{
    gl.Disable(GL_DEPTH_TEST);
    gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    drawLines(aDrawList.mCommands->mLineVertices, aUboRepository, aStorage);
    drawPosedEntry(aDrawList.mCommands->mBoxes, aUboRepository, aStorage);

    gl.Enable(GL_DEPTH_TEST);
    gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

    gl.Disable(GL_DEPTH_TEST);

    gl.DrawArrays(mLines.mPrimitiveMode, 0, (GLsizei)aLines.size());
}


void DebugRenderer::drawPosedEntry(std::span<snac::DebugDrawer::Entry> aEntries,
                                   const RepositoryUbo & aUboRepository,
                                   Storage & aStorage)
{
    std::vector<EntryInstance> instances;
    instances.reserve(aEntries.size());
    for (const auto & entry : aEntries)
    {
        instances.push_back(EntryInstance{
            .mModelTransform = math::trans3d::scale(entry.mScaling)
                                * entry.mOrientation.toRotationMatrix()
                                * math::trans3d::translate(
                                    entry.mPosition.as<math::Vec>()),
            .mColor = entry.mColor,
        });
    }
    proto::load(*mEntryInstanceBuffer, std::span{instances}, graphics::BufferHint::StreamDraw);

    // The VAO might actually be cached as a data member, since it will be constant during runtime
    Handle<graphics::VertexArrayObject> vao = getVao(*mEntryProgram, mBox, aStorage);
    graphics::ScopedBind vaoScope{*vao};
    graphics::ScopedBind programScope{mEntryProgram->mProgram};

    // TODO this looks like something that could be done only once (will not change program side)
    // or at least optimized (the binding points of the UBO might be changed client side, by other code)
    setBufferBackedBlocks(mLineProgram->mProgram, aUboRepository);

    gl.DrawArraysInstanced(mBox.mPrimitiveMode,
                           0,
                           mBox.mVertexCount,
                           (GLsizei)instances.size());
}


} // namespace ad::renderer