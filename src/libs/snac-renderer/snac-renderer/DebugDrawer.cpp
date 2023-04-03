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

    auto effect = loadTrivialEffect(aTechniqueAccess.get("shaders/DebugDraw.prog"));

    DebugDrawer::gSharedData->mCube = loadBox(
        math::Box<float>::UnitCube(),
        effect,
        "debug_box");

    DebugDrawer::gSharedData->mArrow = loadVertices(
        makeArrow(),
        effect,
        "debug_arrow");

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


void DebugDrawer::addArrow(const Entry & aEntry)
{
    mCommands->mArrows.push_back(aEntry);
}


void DebugDrawer::addBasis(Entry aEntry)
{
    // Y
    aEntry.mColor = math::hdr::gGreen<GLfloat>;
    mCommands->mArrows.push_back(aEntry);
    // X
    aEntry.mOrientation *= math::Quaternion<GLfloat>{
        math::UnitVec<3, GLfloat>{{0.f, 0.f, 1.f}},
        math::Degree<GLfloat>{-90.f}
    };
    aEntry.mColor = math::hdr::gRed<GLfloat>;
    mCommands->mArrows.push_back(aEntry);
    // Z
    aEntry.mOrientation *= math::Quaternion<GLfloat>{
        math::UnitVec<3, GLfloat>{{1.f, 0.f, 0.f}},
        math::Degree<GLfloat>{90.f}
    };
    aEntry.mColor = math::hdr::gBlue<GLfloat>;
    mCommands->mArrows.push_back(aEntry);
}


DebugDrawer::DrawList::DrawList() :
    mCommands{std::make_shared<Commands>()},
    mSharedData{nullptr}
{}


void DebugDrawer::DrawList::render(Renderer & aRenderer, ProgramSetup & aSetup) const
{
    auto scopeLineMode = graphics::scopePolygonMode(GL_LINE);

    // TODO this could probably be optimized a lot:
    // * Create complete instance buffer, and draw ranges per mesh
    // * Single draw command glDraw*Indirect()
    // Basically, at the moment we are limited by the API of our Renderer/Pass abstraction
    auto draw = [&, this](std::span<Entry> aEntries, const Mesh & aMesh)
    {
        std::vector<PoseColor> instances;
        instances.reserve(aEntries.size());

        for(const Entry & instance : aEntries)
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

        mPass.draw(aMesh, mSharedData->mInstances, aRenderer, aSetup);
    };

    draw(mCommands->mBoxes, mSharedData->mCube);
    draw(mCommands->mArrows, mSharedData->mArrow);
}


} // namespace snac
} // namespace ad
