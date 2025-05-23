#include "Debug_BoundingBoxes.h"

#include "../component/Tags.h"
#include "../component/GlobalPose.h"
#include "../component/VisualModel.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include "../GameParameters.h"
#include "../GameContext.h"

#include <snacman/DebugDrawing.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {

Debug_BoundingBoxes::Debug_BoundingBoxes(GameContext & aGameContext) :
    mPlayers{aGameContext.mWorld},
    mPills{aGameContext.mWorld}
{}


void Debug_BoundingBoxes::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    using Level = snac::DebugDrawer::Level;

    Level level = Level::off;

// Clang is a little itch, it complains level is captured for no reason when DGBDRAW is disabled
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif

    auto addBox = [&level]
        (const component::GlobalPose & aGlobalPose, const component::VisualModel & aModel) 
        {
            DBGDRAW(snac::gBoundingBoxDrawer, level).addBox(
                snac::DebugDrawer::Entry{
                    .mPosition = aGlobalPose.mPosition,
                    .mScaling = aGlobalPose.mInstanceScaling * aGlobalPose.mScaling,
                    .mOrientation = aGlobalPose.mOrientation,
                },
                aModel.mModel->mAabb
            );
        };

    level = Level::info;
    mPlayers.each(addBox);
    level = Level::trace;
    mPills.each(addBox);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    mPlayers.each(
        [](const component::GlobalPose & aGlobalPose)
        {
            DBGDRAW_INFO(snac::gBoundingBoxDrawer).addText(
                snac::DebugDrawer::Text{
                    .mPosition = aGlobalPose.mPosition,
                    .mMessage = "Player",
                }
            );
        });


    DBGDRAW_INFO(snac::gWorldDrawer).addLine({0.f, 0.f, 0.f}, {0.f, 10.f, 0.f});
    DBGDRAW_DEBUG(snac::gWorldDrawer).addPlane(
        {0.f, 0.f, -15.f},
        {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
        40, 10,
        20.f, 5.f);

    // Also add the world basis
    DBGDRAW_INFO(snac::gWorldDrawer).addBasis({.mOrientation = gWorldCoordTo3dCoordQuat});
}


} // namespace system
} // namespace snacgame
} // namespace ad
