#pragma once

#include "../InputConstants.h"

namespace ad {
namespace snacgame {
namespace component {

struct PlayerMoveState
{
    int mMoveState = gPlayerMoveFlagNone;

    void drawUi() const;
};


} // namespace component
} // namespace snacgame
} // namespace ad
