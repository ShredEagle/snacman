#pragma once


#include "../InputCommandConverter.h"
namespace ad {
namespace snacgame {
namespace component {

constexpr int gAllowedMovementNone = gPlayerMoveFlagNone;
constexpr int gAllowedMovementUp = gPlayerMoveFlagUp;
constexpr int gAllowedMovementDown = gPlayerMoveFlagDown;
constexpr int gAllowedMovementLeft = gPlayerMoveFlagLeft;
constexpr int gAllowedMovementRight = gPlayerMoveFlagRight;

struct AllowedMovement
{
    int mAllowedMovement = 0;
};

} // namespace component
} // namespace snacgame
} // namespace ad
