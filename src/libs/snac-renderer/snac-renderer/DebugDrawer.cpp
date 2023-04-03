#include "DebugDrawer.h"

#include "Cube.h"
#include "Instances.h"
#include "ResourceLoad.h"

#include <math/Transformations.h>


namespace ad {
namespace snac {


std::unique_ptr<DebugDrawer::SharedData> DebugDrawer::gSharedData;


Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess)
{
    static DebugDrawer::SharedData sharedData;
    DebugDrawer::gSharedData = std::make_unique<DebugDrawer::SharedData>();

    DebugDrawer::gSharedData->mCube = loadBox(
        math::Box<float>::UnitCube(),
        loadTrivialEffect(aTechniqueAccess.get("shaders/DebugDraw.prog")),
        "debug_box");

    DebugDrawer::gSharedData->mInstances = initializeInstanceStream<PoseColor>();

    return Guard{[]()
    {
        DebugDrawer::gSharedData = nullptr;
    }};
}


void DebugDrawer::startFrame()
{
    mCommands = std::make_shared<Commands>();
}


void DebugDrawer::addBox(const Entry & aEntry)
{
    mCommands->mBoxes.push_back(aEntry);
}


void DebugDrawer::addBox(Entry aEntry, const math::Box<GLfloat> aBox)
{
    // The translation transformation (from the instance position) will be applied last.
    // It needs to take into account the scaling and rotation that will already be applied.
    aEntry.mPosition += 
        aEntry.mOrientation.rotate(
            aBox.origin().cwMul(aEntry.mScaling.as<math::Position>()).as<math::Vec>());
    aEntry.mScaling.cwMulAssign(aBox.dimension());

    addBox(aEntry);
}


DebugDrawer::DrawList::DrawList() :
    mCommands{std::make_shared<Commands>()},
    mSharedData{nullptr}
{}


void DebugDrawer::DrawList::render(Renderer & aRenderer, ProgramSetup & aSetup) const
{
    auto scopeLineMode = graphics::scopePolygonMode(GL_LINE);

    std::vector<PoseColor> instances;
    instances.reserve(mCommands->mBoxes.size());

    for(const Entry & instance : mCommands->mBoxes)
    {
        instances.push_back(PoseColor{
                .pose = math::trans3d::scale(instance.mScaling)
                        * instance.mOrientation.toRotationMatrix()
                        * math::trans3d::translate(
                            instance.mPosition.as<math::Vec>()),
                .albedo = to_sdr(instance.mColor),
            });
    }
    mSharedData->mInstances.respecifyData(std::span{instances});

    mPass.draw(mSharedData->mCube, mSharedData->mInstances, aRenderer, aSetup);
}


} // namespace snac
} // namespace ad
