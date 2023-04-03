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


DebugDrawer::Commands & DebugDrawer::commands(Level aLevelSanityCheck)
{
    // Ideally, the filter is checked first thing in each "drawing" method
    // So we should never be here with a level not passing the filter.
    assert(passDrawFilter(aLevelSanityCheck));

    return *mRegistry->mFrameCommands;
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


void DebugDrawer::addBox(Level aLevel, Entry aEntry, const math::Box<float> aBox)
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
}


DebugDrawer::DrawList::DrawList() :
    mCommands{std::make_shared<Commands>()}
{}


} // namespace snac
} // namespace ad
