#pragma once

#include "LevelTags.h"
#include "../InputConstants.h"

#include <platform/Filesystem.h>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <markovjunior/Grid.h>
#include <math/Matrix.h>

namespace ad {
namespace snacgame {
namespace component {

constexpr int gAllowedMovementNone = gPlayerMoveFlagNone;
constexpr int gAllowedMovementUp = gPlayerMoveFlagUp;
constexpr int gAllowedMovementDown = gPlayerMoveFlagDown;
constexpr int gAllowedMovementLeft = gPlayerMoveFlagLeft;
constexpr int gAllowedMovementRight = gPlayerMoveFlagRight;

enum class TileType
{
    Void,
    Path,
    Portal,
    Pen,
    Spawn,
};

struct Tile
{
    TileType mType;
    int mAllowedMove = gAllowedMovementNone;
};

struct LevelData
{
    // Level grid stored as
    // 0 1 2 ... mColCount - 1 (0th row)
    // mColCount mColCount + 1 ... 2 * mColCount - 1 (1th row)
    // ...
    // mColCount * (mRowCount - 1) ... (mColCount * mRowCount) - 1 (mRowCountth
    // row)
    LevelData(ent::EntityManager & aWorld,
              const filesystem::path & aAssetRoot,
              const filesystem::path & aFile,
              const math::Size<3, int> & aSize,
              int aSeed) :
        mAssetRoot{aAssetRoot}, mFile{aFile}, mSize{aSize}, mSeed{aSeed}
    {}

    filesystem::path mAssetRoot;
    filesystem::path mFile;
    math::Size<3, int> mSize;
    int mSeed;

    std::vector<Tile> mTiles;
    float mCellSize = 1.f;
};

} // namespace component
} // namespace snacgame
} // namespace ad
