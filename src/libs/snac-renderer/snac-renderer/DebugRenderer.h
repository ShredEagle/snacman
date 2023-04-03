#pragma once


#include "DebugDrawer.h"
#include "Mesh.h"
#include "Render.h"


namespace ad {
namespace snac {



/// @brief Used to render on screen the debug DrawList.
class DebugRenderer
{
public:
    DebugRenderer(Load<Technique> & aTechniqueAccess);

    void render(DebugDrawer::DrawList aDrawList, Renderer & aRenderer, ProgramSetup & aSetup);

private:
    Mesh mCube;
    Mesh mArrow;
    Pass mPass{"DebugDrawing"};
    InstanceStream mInstances;
};


} // namespace snac
} // namespace ad
