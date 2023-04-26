#include "DebugDrawer.h"

#include "Logging.h"

#include <math/Transformations.h>


#define SELOG(severity) SELOG_LG(::ad::snac::gRenderLogger, severity)


namespace ad {
namespace snac {


const std::string & to_string(DebugDrawer::Level aLevel)
{
    return DebugDrawer::gLevelStrings[static_cast<std::size_t>(aLevel)];
}


void DebugDrawer::Registry::startFrame()
{
    mFrameCommands = std::make_shared<Commands>();
}
    

DebugDrawer::DrawList DebugDrawer::Registry::endFrame()
{ 
    return{std::move(mFrameCommands)};
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


DebugDrawer::Commands & DebugDrawer::commands()
{
    return *mRegistry->mFrameCommands;
}


bool DebugDrawer::shouldDraw(Level aLevel) const
{
    return getLevel() <= aLevel;
}


void DebugDrawer::addBox(const Entry & aEntry)
{
    commands().mBoxes.push_back(aEntry);
}


void DebugDrawer::addBox(Entry aEntry, const math::Box<float> aBox)
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
    commands().mArrows.push_back(aEntry);
}


void DebugDrawer::addBasis(Entry aEntry)
{
    auto & cmds = commands();
    // Y
    aEntry.mColor = math::hdr::gGreen<float>;
    cmds.mArrows.push_back(aEntry);
    // X
    aEntry.mOrientation *= math::Quaternion<float>{
        math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
        math::Degree<float>{-90.f}
    };
    aEntry.mColor = math::hdr::gRed<float>;
    cmds.mArrows.push_back(aEntry);
    // Z
    aEntry.mOrientation *= math::Quaternion<float>{
        math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
        math::Degree<float>{90.f}
    };
    aEntry.mColor = math::hdr::gBlue<float>;
    cmds.mArrows.push_back(aEntry);
}


void DebugDrawer::addText(Text aText)
{
    commands().mTexts.push_back(std::move(aText));
}


void DebugDrawer::addLine(const LineVertex & aP1, const LineVertex & aP2)
{
    commands().mLineVertices.push_back(std::move(aP1));
    commands().mLineVertices.push_back(std::move(aP2));
}


void DebugDrawer::addPlane(math::Position<3, float> aOrigin,
                           math::Vec<3, float> aDir1, math::Vec<3, float> aDir2,
                           int aSubdiv1, int aSubdiv2,
                           float aSize1, float aSize2,
                           math::hdr::Rgba_f aOutlineColor,
                           math::hdr::Rgba_f aSubdivColor)
{
    // Implementation from 3DRC (1st) Ch4
    // Draw the outlines
    addLine(aOrigin - aSize1 / 2.0f * aDir1 - aSize2 / 2.0f * aDir2,
            aOrigin - aSize1 / 2.0f * aDir1 + aSize2 / 2.0f * aDir2,
            aOutlineColor);
    addLine(aOrigin + aSize1 / 2.0f * aDir1 - aSize2 / 2.0f * aDir2,
            aOrigin + aSize1 / 2.0f * aDir1 + aSize2 / 2.0f * aDir2,
            aOutlineColor);
    addLine(aOrigin - aSize1 / 2.0f * aDir1 + aSize2 / 2.0f * aDir2,
            aOrigin + aSize1 / 2.0f * aDir1 + aSize2 / 2.0f * aDir2,
            aOutlineColor);
    addLine(aOrigin - aSize1 / 2.0f * aDir1 - aSize2 / 2.0f * aDir2,
            aOrigin + aSize1 / 2.0f * aDir1 - aSize2 / 2.0f * aDir2,
            aOutlineColor);


    // Draw internal subdivisions
    for(int i = 1 ; i < aSubdiv1 ; i++) 
    {
        const float t = ((float)i - (float)aSubdiv1 / 2.0f) * aSize1 / (float)aSubdiv1;
        const auto o1 = aOrigin + t * aDir1;
        addLine(o1 - aSize2 / 2.0f * aDir2,
                o1 + aSize2 / 2.0f * aDir2,
                aSubdivColor);
    }

    for(int i = 1 ; i < aSubdiv2 ; i++)
    {
        const float t = ((float)i - (float)aSubdiv2 / 2.0f) * aSize2 / (float)aSubdiv2;
        const auto o2 = aOrigin + t * aDir2;
        addLine(o2 - aSize1 / 2.0f * aDir1,
                o2 + aSize1 / 2.0f * aDir1,
                aSubdivColor);
    }
}


DebugDrawer::DrawList::DrawList() :
    mCommands{std::make_shared<Commands>()}
{}


} // namespace snac
} // namespace ad
