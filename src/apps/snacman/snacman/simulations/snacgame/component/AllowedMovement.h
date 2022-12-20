#pragma once


namespace ad {
namespace snacgame {
namespace component {

typedef int AllowedMovementFlag;

constexpr AllowedMovementFlag AllowedMovementUp = 0x1;
constexpr AllowedMovementFlag AllowedMovementDown = 0x2;
constexpr AllowedMovementFlag AllowedMovementLeft = 0x4;
constexpr AllowedMovementFlag AllowedMovementRight = 0x8;

struct AllowedMovement
{
    AllowedMovementFlag mAllowedMovement = 0;
};

} // namespace component
} // namespace snacgame
} // namespace ad
