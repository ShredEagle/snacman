#pragma once

#include "../InputConstants.h"

namespace ad {
namespace snacgame {
namespace component {

struct PlayerMoveState
{
    int mMoveState = gPlayerMoveFlagNone;
    int mAllowedMove = gPlayerMoveFlagDown;
    int mCurrentPortal = -1;
    int mDestinationPortal = -1;

    void drawUi() const;
};


} // namespace component
} // namespace snacgame
} // namespace ad
