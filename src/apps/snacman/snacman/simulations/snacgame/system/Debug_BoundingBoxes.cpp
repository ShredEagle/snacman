#include "Debug_BoundingBoxes.h"

#include <snacman/DebugDrawing.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {


void Debug_BoundingBoxes::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    using Level = snac::DebugDrawer::Level;

    Level level = Level::off;

    auto addBox = [this, &level]
        (const component::GlobalPose & aGlobalPose, const component::VisualModel & aModel) 
        {
            SEDBGDRAW(snac::gBoundingBoxDrawer)->addBox(
                level,
                snac::DebugDrawer::Entry{
                    .mPosition = aGlobalPose.mPosition,
                    .mScaling = aGlobalPose.mInstanceScaling,
                    .mOrientation = aGlobalPose.mOrientation,
                },
                aModel.mModel->mBoundingBox
            );
        };

    level = Level::info;
    mPlayers.each(addBox);
    level = Level::debug;
    mPills.each(addBox);

    mPlayers.each(
        [](const component::GlobalPose & aGlobalPose)
        {
            SEDBGDRAW(snac::gBoundingBoxDrawer)->addText(
                Level::info,
                snac::DebugDrawer::Text{
                    .mPosition = aGlobalPose.mPosition,
                    .mMessage = "Player",
                }
            );
        });


    SEDBGDRAW(snac::gWorldDrawer)->addLine(Level::info, {0.f, 0.f, 0.f}, {0.f, 10.f, 0.f});
    SEDBGDRAW(snac::gWorldDrawer)->addPlane(Level::info,
        {0.f, 0.f, -15.f},
        {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
        40, 10,
        20.f, 5.f);

    // Also add the world basis
    SEDBGDRAW(snac::gWorldDrawer)->addBasis(Level::info, {});
}


} // namespace system
} // namespace snacgame
} // namespace ad
