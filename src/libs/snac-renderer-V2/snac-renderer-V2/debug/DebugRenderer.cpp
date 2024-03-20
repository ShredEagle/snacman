#include "DebugRenderer.h"

#include "../Pass.h"
#include "../Semantics.h"
#include "../SetupDrawing.h"
#include "../VertexStreamUtilities.h"

#include "../files/Loader.h"

namespace ad::renderer {


namespace {

    static const std::array<AttributeDescription, 2> gLineAttributes {
        AttributeDescription{
            .mSemantic = semantic::gPosition,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gColor,
            .mDimension = 4,
            .mComponentType = GL_FLOAT,
        },
    };

} // unnamed namespace


DebugRenderer::DebugRenderer(Storage & aStorage, const Loader & aLoader) :
    mLines{
        .mName = "DebugLines",
        // TODO It would be more semantically correct to host the "mLineBuffer" ourselves,
        // and use a (to be implemented) function that would create the vertex stream *around* 
        // this buffer we are explicitly responsible for.
        .mVertexStream = makeVertexStream(0, 0, gLineAttributes, aStorage, {}),
        .mPrimitiveMode = GL_LINES,
    },
    mLinePositionBuffer{getBufferView(*mLines.mVertexStream, semantic::gPosition).mGLBuffer},
    mLineColorBuffer{getBufferView(*mLines.mVertexStream, semantic::gColor).mGLBuffer},
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
    // TODO enable interleaving so we can use the aLines span directly
    std::vector<math::Position<3, float>> positions;
    std::vector<math::hdr::Rgba<float>> colors;
    for(const auto & lineVertex : aLines)
    {
        positions.push_back(lineVertex.mPosition);
        colors.push_back(lineVertex.mColor);
    }

    {
        std::span lineData{positions};
        glBindBuffer(GL_ARRAY_BUFFER, *mLinePositionBuffer);
        glBufferData(GL_ARRAY_BUFFER, lineData.size_bytes(), lineData.data(), GL_STREAM_DRAW);
    }
    {
        std::span lineData{colors};
        glBindBuffer(GL_ARRAY_BUFFER, *mLineColorBuffer);
        glBufferData(GL_ARRAY_BUFFER, lineData.size_bytes(), lineData.data(), GL_STREAM_DRAW);
    }

    // The VAO might actually be cached as a data member, since it will be constant during runtime
    Handle<graphics::VertexArrayObject> vao = getVao(*mLineProgram, mLines, aStorage);
    graphics::ScopedBind vaoScope{*vao};
    graphics::ScopedBind programScope{mLineProgram->mProgram};

    setBufferBackedBlocks(mLineProgram->mProgram, aUboRepository);

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(mLines.mPrimitiveMode, 0, (GLsizei)aLines.size());
}


} // namespace ad::renderer