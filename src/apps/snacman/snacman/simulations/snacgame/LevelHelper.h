#pragma once

#include "GameParameters.h"
#include "snacman/simulations/snacgame/InputConstants.h"

#include <cmath>
#include <math/Vector.h>

namespace ad {
using namespace math;
namespace snacgame {

constexpr int gAllowedMovementNone = gPlayerMoveFlagNone;
constexpr int gAllowedMovementUp = gPlayerMoveFlagUp;
constexpr int gAllowedMovementDown = gPlayerMoveFlagDown;
constexpr int gAllowedMovementLeft = gPlayerMoveFlagLeft;
constexpr int gAllowedMovementRight = gPlayerMoveFlagRight;


inline Position<2, float> getLevelPosition(const Position<2, float> & aPos)
{
    return Position<2, float>{std::floor(aPos.x() + gCellSize / 2),
                              std::floor(aPos.y() + gCellSize / 2)};
}

inline Position<2, int> getLevelPosition_i(const Position<2, float> & aPos)
{
    return Position<2, int>{static_cast<int>(aPos.x() + gCellSize / 2),
                            static_cast<int>(aPos.y() + gCellSize / 2)};
}

constexpr std::array<Position<2, int>, 4> gDirections{
    Position<2, int>{0, 1}, Position<2, int>{0, -1},
    Position<2, int>{1, 0}, Position<2, int>{-1, 0}};

constexpr std::array<const int, 4> gAllowedMovement{
    gAllowedMovementUp, gAllowedMovementDown,
    gAllowedMovementRight, gAllowedMovementLeft};

inline float manhattan(const math::Position<2, float> & aA,
                       const math::Position<2, float> & aB)
{
    return std::abs(aA.x() - aB.x()) + std::abs(aA.y() - aB.y());
}

} // namespace snacgame
} // namespace ad
