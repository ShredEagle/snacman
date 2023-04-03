#include "DebugDrawer.h"

#include "Cube.h"
#include "Instances.h"
#include "Logging.h"
#include "ResourceLoad.h"

#include <math/Transformations.h>


#define SELOG(severity) SELOG_LG(::ad::snac::gRenderLogger, severity)


namespace ad {
namespace snac {


std::unique_ptr<DebugDrawer::SharedData> DebugDrawer::Registry::gSharedData;


Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess)
{
    DebugDrawer::Registry::gSharedData = std::make_unique<DebugDrawer::SharedData>();
    std::unique_ptr<DebugDrawer::SharedData> & sharedData = DebugDrawer::Registry::gSharedData;


    auto effect = loadTrivialEffect(aTechniqueAccess.get("shaders/DebugDraw.prog"));

    sharedData->mCube = loadBox(
        math::Box<float>::UnitCube(),
        effect,
        "debug_box");

    sharedData->mArrow = loadVertices(
        makeArrow(),
        effect,
        "debug_arrow");

    sharedData->mInstances = initializeInstanceStream<PoseColor>();

    return Guard{[]()
    {
        DebugDrawer::Registry::gSharedData = nullptr;
    }};
}


const std::string & to_string(DebugDrawer::Level aLevel)
{
    return DebugDrawer::gLevelStrings[static_cast<std::size_t>(aLevel)];
}


void DebugDrawer::Registry::startFrame()
{
    mCommands = std::make_shared<Commands>();
}
    

DebugDrawer::DrawList DebugDrawer::Registry::endFrame()
{ 
    return{
        std::move(mCommands),
        gSharedData.get()
    };
}


std::shared_ptr<DebugDrawer> DebugDrawer::Registry::addDrawer(const std::string & aName)
{
    auto [drawer, didInsert] = mDrawers.emplace(aName, std::make_shared<DebugDrawer>(this, mLevel));
    if(!didInsert)
    {
        SELOG(warn)("DebugDrawer name '{}' already registered.", aName);
    }
    return drawer->second;
}


std::shared_ptr<DebugDrawer> DebugDrawer::Registry::get(const std::string & aName) const
{
    auto found = mDrawers.find(aName);
    if(found == mDrawers.end())
    {
        SELOG(error)("No DebugDrawer named '{}'.", aName);
        throw std::invalid_argument{"Requested DebugDrawer is not present."};
    }
    else
    {
        return found->second;
    }
}


void DebugDrawer::Registry::setLevel(Level aLevel)
{
    mLevel = aLevel;
    for(auto [_name, drawer] : mDrawers)
    {
        drawer->getLevel() = aLevel;
    }
}


DebugDrawer::Commands & DebugDrawer::commands(Level aLevelSanityCheck)
{
    // Ideally, the filter is checked first thing in each "drawing" method
    // So we should never be here with a level not passing the filter.
    assert(passDrawFilter(aLevelSanityCheck));

    return *mRegistry->mCommands;
}


bool DebugDrawer::passDrawFilter(Level aDrawLevel)
{
    assert(static_cast<int>(aDrawLevel) >= 0 && aDrawLevel < Level::off);
    return aDrawLevel >= mLevel;
}


void DebugDrawer::addBox(Level aLevel, const Entry & aEntry)
{
    if(passDrawFilter(aLevel))
    {
        commands(aLevel).mBoxes.push_back(aEntry);
    }
}


void DebugDrawer::addBox(Level aLevel, Entry aEntry, const math::Box<GLfloat> aBox)
{
    // TODO this filter design is not satisfying. Here, passDrawFilter will be invoked 3 times.
    if(passDrawFilter(aLevel))
    {
        // The translation transformation (from the instance position) will be applied last.
        // It needs to take into account the scaling and rotation that will already be applied.
        aEntry.mPosition += 
            aEntry.mOrientation.rotate(
                aBox.origin().cwMul(aEntry.mScaling.as<math::Position>()).as<math::Vec>());
        aEntry.mScaling.cwMulAssign(aBox.dimension());

        addBox(aLevel, aEntry);
    }
}


void DebugDrawer::addArrow(Level aLevel, const Entry & aEntry)
{
    if(passDrawFilter(aLevel))
    {
        commands(aLevel).mArrows.push_back(aEntry);
    }
}


void DebugDrawer::addBasis(Level aLevel, Entry aEntry)
{
    if(passDrawFilter(aLevel))
    {
        auto & cmds = commands(aLevel);
        // Y
        aEntry.mColor = math::hdr::gGreen<GLfloat>;
        cmds.mArrows.push_back(aEntry);
        // X
        aEntry.mOrientation *= math::Quaternion<GLfloat>{
            math::UnitVec<3, GLfloat>{{0.f, 0.f, 1.f}},
            math::Degree<GLfloat>{-90.f}
        };
        aEntry.mColor = math::hdr::gRed<GLfloat>;
        cmds.mArrows.push_back(aEntry);
        // Z
        aEntry.mOrientation *= math::Quaternion<GLfloat>{
            math::UnitVec<3, GLfloat>{{1.f, 0.f, 0.f}},
            math::Degree<GLfloat>{90.f}
        };
        aEntry.mColor = math::hdr::gBlue<GLfloat>;
        cmds.mArrows.push_back(aEntry);
    }
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
