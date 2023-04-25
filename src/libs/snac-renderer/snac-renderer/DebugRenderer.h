#pragma once


#include "DebugDrawer.h"
#include "Mesh.h"
#include "Render.h"
#include "text/Text.h"
#include "text/TextRenderer.h"


namespace ad {
namespace snac {



/// @brief Used to render on screen the debug DrawList.
class DebugRenderer
{
public:
    DebugRenderer(Load<Technique> & aTechniqueAccess, arte::FontFace aFontFace);

    void render(DebugDrawer::DrawList aDrawList, Renderer & aRenderer, ProgramSetup & aSetup);

private:
    void drawLines(std::span<DebugDrawer::LineVertex> aLines, Renderer & aRenderer, ProgramSetup & aSetup);

    void drawText(std::span<DebugDrawer::Text> aTexts, Renderer & aRenderer, ProgramSetup & aSetup);

    Mesh mCube;
    Mesh mArrow;
    Pass mPass{"DebugDrawing"};
    InstanceStream mInstances;
    Font mFont;
    snac::IntrospectProgram mLineProgram;
    snac::VertexStream mLineStream;
    snac::TextRenderer mTextRenderer;
    snac::GlyphInstanceStream mStrings;
};


} // namespace snac
} // namespace ad
