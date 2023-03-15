#pragma once

namespace ad {
namespace snacgame {
constexpr int gPlayerMoveFlagNone = 0;

constexpr int gPositiveEdge = 1 << 0;

constexpr int gPlayerMoveFlagUp = 1 << 1;
constexpr int gPlayerMoveFlagDown = 1 << 2;
constexpr int gPlayerMoveFlagLeft = 1 << 3;
constexpr int gPlayerMoveFlagRight = 1 << 4;

constexpr int gJoin = 1 << 5;

constexpr int gSelectItem = 1 << 6;
constexpr int gBack = 1 << 7;
constexpr int gGoUp = 1 << 8;
constexpr int gGoDown = 1 << 9;
constexpr int gGoLeft = 1 << 10;
constexpr int gGoRight = 1 << 11;

constexpr int gQuitCommand = 1 << 12;
}
}
