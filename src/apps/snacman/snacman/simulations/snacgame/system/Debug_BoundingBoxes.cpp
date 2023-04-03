#include "Debug_BoundingBoxes.h"

#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {


void Debug_BoundingBoxes::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    auto addBox = [this]
        (const component::GlobalPose & aGlobalPose, const component::VisualModel & aModel) 
        {
            mGameContext->mDebugDrawer.addBox(
                snac::DebugDrawer::Entry{
                    .mPosition = aGlobalPose.mPosition,
                    .mScaling = aGlobalPose.mInstanceScaling,
                    .mOrientation = aGlobalPose.mOrientation,
                },
                aModel.mModel->mBoundingBox
            );
        };

    mPlayers.each(addBox);
    mPills.each(addBox);
}


} // namespace system
} // namespace snacgame
} // namespace ad
