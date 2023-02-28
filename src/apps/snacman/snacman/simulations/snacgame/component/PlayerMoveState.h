#pragma once

#include "../InputCommandConverter.h"

namespace ad {
namespace snacgame {
namespace component {

struct PlayerMoveState
{
    int mMoveState = gPlayerMoveFlagNone;
    int mAllowedMove = gPlayerMoveFlagDown;
};


} // namespace component
} // namespace snacgame
} // namespace ad
