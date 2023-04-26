#include "DebugRenderer.h"

#include "Cube.h"
#include "Instances.h"
#include "ResourceLoad.h"

#include <math/Transformations.h>


namespace ad {
namespace snac {


namespace {

    snac::VertexStream prepareLineVertexStream()
    {
        using LineVertex = DebugDrawer::LineVertex;

        snac::VertexStream stream{
            .mPrimitive = GL_LINES,
        };

        stream.mVertexBuffers.push_back(BufferView{.mStride = sizeof(LineVertex)});

        {
            graphics::ClientAttribute vertexPosition{
                .mDimension = 3,
                .mOffset = offsetof(LineVertex, mPosition),
                .mComponentType = GL_FLOAT,
            };

            stream.mAttributes.emplace(Semantic::Position, AttributeAccessor{0, vertexPosition});
        }

        {
            graphics::ClientAttribute color{
                .mDimension = 4,
                .mOffset = offsetof(LineVertex, mColor),
                .mComponentType = GL_FLOAT,
            };

            stream.mAttributes.emplace(Semantic::Albedo, AttributeAccessor{0, color});
        }

        return stream;
    }

} // unnamed

// TODO I do not like that it needs to get the font instead of using a Load<> interface
// Yet the font loading needs to take the effect file as a second argument,
// and the load interface does not support extra parameters.
DebugRenderer::DebugRenderer(Load<Technique> & aTechniqueAccess, arte::FontFace aFontFace) :
    mLineStream{prepareLineVertexStream()},
    mLineProgram{std::move(aTechniqueAccess.get("shaders/DebugDraw.prog").mProgram)},
    mFont{
        std::move(aFontFace),
        48,
        loadTrivialEffect(aTechniqueAccess.get("shaders/BillboardText.prog"))}
{
    auto effect = loadTrivialEffect(aTechniqueAccess.get("shaders/DebugDrawModelTransform.prog"));

    mCube = loadBox(
        math::Box<float>{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}},
        effect,
        "debug_box");

    mArrow = loadVertices(
        makeArrow(),
        effect,
        "debug_arrow");

    mInstances = initializeInstanceStream<PoseColor>();
}


void DebugRenderer::render(DebugDrawer::DrawList aDrawList, Renderer & aRenderer, ProgramSetup & aSetup)
{
    {
        auto scopeLineMode = graphics::scopePolygonMode(GL_LINE);

        // TODO this could probably be optimized a lot:
        // * Create complete instance buffer, and draw ranges per mesh
        // * Single draw command glDraw*Indirect()
        // Basically, at the moment we are limited by the API of our Renderer/Pass abstraction
        auto draw = [&, this](std::span<DebugDrawer::Entry> aEntries, const Mesh & aMesh)
        {
            std::vector<PoseColor> instances;
            instances.reserve(aEntries.size());

            for(const DebugDrawer::Entry & instance : aEntries)
            {
                instances.push_back(PoseColor{
                        .pose = math::trans3d::scale(instance.mScaling)
                                * instance.mOrientation.toRotationMatrix()
                                * math::trans3d::translate(
                                    instance.mPosition.as<math::Vec>()),
                        .albedo = to_sdr(instance.mColor),
                    });
            }
            mInstances.respecifyData(std::span{instances});

            mPass.draw(aMesh, mInstances, aRenderer, aSetup);
        };

        draw(aDrawList.mCommands->mBoxes, mCube);
        draw(aDrawList.mCommands->mArrows, mArrow);
    }

    drawLines(aDrawList.mCommands->mLineVertices, aRenderer, aSetup);

    drawText(aDrawList.mCommands->mTexts, aRenderer, aSetup);
}


void DebugRenderer::drawLines(std::span<DebugDrawer::LineVertex> aLines, Renderer & aRenderer, ProgramSetup & aSetup)
{
    assert(mLineStream.mVertexBuffers.size() == 1);
    mLineStream.respecifyData(0, aLines);

    aRenderer.setupProgram(mPass.getName(), mLineProgram, aSetup);
    aRenderer.draw(mLineStream, snac::gNotInstanced, mLineProgram);
}


void DebugRenderer::drawText(std::span<DebugDrawer::Text> aTexts, Renderer & aRenderer, ProgramSetup & aSetup)
{
    std::vector<snac::GlyphInstance> glyphs;
    for (const DebugDrawer::Text & text : aTexts)
    {
        auto translation = math::trans3d::translate(text.mPosition.as<math::Vec>());
        auto glyphInstances = 
            mFont.mFontData.populateInstances(text.mMessage,
                                               to_sdr(text.mColor),
                                               translation);
        glyphs.insert(glyphs.end(), glyphInstances.begin(), glyphInstances.end());
    }

    mStrings.respecifyData(std::span{glyphs});
    mTextRenderer.render(mStrings, mFont, aRenderer, aSetup);
}


} // namespace snac
} // namespace ad
