#pragma once

namespace ad {
namespace snacgame {
namespace component {

struct Pill
{};

struct LevelTile
{};

// Entity that should be deleted during
// a round transition
struct RoundTransient
{};

// Entity that should be deleted on
// game scene teardown
struct GameTransient
{};

struct Crown
{};

} // namespace component
} // namespace snacgame
} // namespace ad
