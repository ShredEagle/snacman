#include "DebugRenderer.h"

#include "../Pass.h"
#include "../RendererReimplement.h"
#include "../Semantics.h"
#include "../SetupDrawing.h"
#include "../VertexStreamUtilities.h"

#include "../files/Loader.h"

namespace ad::renderer {


namespace {
    using LineVertex = snac::DebugDrawer::LineVertex;

    static const std::array<std::pair<AttributeDescription, std::size_t>, 2> gLineAttributes {{
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

    Handle<VertexStream> setupLineVertexStream(Storage & aStorage,
                                            Handle<const graphics::BufferAny> aVertexBuffer)
    {
        Handle<VertexStream> result = primeVertexStream(aStorage);
        addInterleavedVertexAttributes(
            result,
            sizeof(snac::DebugDrawer::LineVertex),
            gLineAttributes,
            aVertexBuffer,
            0 // No vertices on construction : the buffer will be loaded on rendering
        );
        return result;
    }

} // unnamed namespace


DebugRenderer::DebugRenderer(Storage & aStorage, const Loader & aLoader) :
    mLineVertexBuffer{makeBuffer(0, 0, GL_STREAM_DRAW, aStorage)},
    mLines{
        .mName = "DebugLines",
        .mVertexStream = setupLineVertexStream(aStorage, mLineVertexBuffer),
        .mPrimitiveMode = GL_LINES,
    },
    mLineProgram{storeConfiguredProgram(aLoader.loadProgram("shaders/DebugDraw.prog"), aStorage)}
{}

void DebugRenderer::render(snac::DebugDrawer::DrawList aDrawList,
                           const RepositoryUbo & aUboRepository,
                           Storage & aStorage)
{
    drawLines(aDrawList.mCommands->mLineVertices, aUboRepository, aStorage);
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

    setBufferBackedBlocks(mLineProgram->mProgram, aUboRepository);

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(mLines.mPrimitiveMode, 0, (GLsizei)aLines.size());
}


} // namespace ad::renderer