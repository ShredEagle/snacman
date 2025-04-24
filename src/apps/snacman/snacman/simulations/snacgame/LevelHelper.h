#pragma once

#include "GameParameters.h"
#include "snacman/simulations/snacgame/InputConstants.h"

#include <cmath>
#include <math/Vector.h>

namespace ad {
namespace snacgame {

constexpr int gAllowedMovementNone = gPlayerMoveFlagNone;
constexpr int gAllowedMovementUp = gPlayerMoveFlagUp;
constexpr int gAllowedMovementDown = gPlayerMoveFlagDown;
constexpr int gAllowedMovementLeft = gPlayerMoveFlagLeft;
constexpr int gAllowedMovementRight = gPlayerMoveFlagRight;


inline math::Position<2, float> getLevelPosition(const math::Position<2, float> & aPos)
{
    return math::Position<2, float>{std::floor(aPos.x() + gCellSize / 2),
                              std::floor(aPos.y() + gCellSize / 2)};
}

inline math::Position<2, int> getLevelPosition_i(const math::Position<2, float> & aPos)
{
    return math::Position<2, int>{static_cast<int>(aPos.x() + gCellSize / 2),
                            static_cast<int>(aPos.y() + gCellSize / 2)};
}

enum Direction {
    Up,
    Down,
    Left,
    Right
};

constexpr std::array<math::Vec<2, int>, 4> gDirections{
    math::Vec<2, int>{0, 1}, math::Vec<2, int>{0, -1},
    math::Vec<2, int>{1, 0}, math::Vec<2, int>{-1, 0}};

constexpr std::array<math::Vec<2, float>, 4> gDirections_f{
    math::Vec<2, float>{0.f, 1.f}, math::Vec<2, float>{0.f, -1.f},
    math::Vec<2, float>{1.f, 0.f}, math::Vec<2, float>{-1.f, 0.f}};

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
