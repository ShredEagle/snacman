#pragma once

namespace ad {
namespace snacgame {
constexpr int gNoCommand = -1;
constexpr int gPlayerMoveFlagNone = 0;

constexpr int gPositiveEdge = 1 << 0;

constexpr int gPlayerMoveFlagUp = 1 << 1;
constexpr int gPlayerMoveFlagDown = 1 << 2;
constexpr int gPlayerMoveFlagLeft = 1 << 3;
constexpr int gPlayerMoveFlagRight = 1 << 4;
constexpr int gPlayerUsePowerup = 1 << 5;

constexpr int gJoin = 1 << 10;

constexpr int gSelectItem = 1 << 16;
constexpr int gBack = 1 << 17;
constexpr int gGoUp = 1 << 18;
constexpr int gGoDown = 1 << 19;
constexpr int gGoLeft = 1 << 20;
constexpr int gGoRight = 1 << 21;

constexpr int gQuitCommand = 1 << 22;

constexpr int gRightJoyUp = 1 << 23;
constexpr int gRightJoyDown = 1 << 24;
constexpr int gRightJoyLeft = 1 << 25;
constexpr int gRightJoyRight = 1 << 26;

constexpr int gBaseTargetShift = 27;
constexpr int gPrevPowerUpTarget = 1 << gBaseTargetShift;
constexpr int gNextPowerUpTarget = 1 << (gBaseTargetShift + 1);
}
}
