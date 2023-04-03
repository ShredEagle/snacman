#include "DebugRenderer.h"

#include "Cube.h"
#include "Instances.h"
#include "ResourceLoad.h"

#include <math/Transformations.h>


namespace ad {
namespace snac {


DebugRenderer::DebugRenderer(Load<Technique> & aTechniqueAccess)
{
    auto effect = loadTrivialEffect(aTechniqueAccess.get("shaders/DebugDraw.prog"));

    mCube = loadBox(
        math::Box<float>::UnitCube(),
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


} // namespace snac
} // namespace ad
