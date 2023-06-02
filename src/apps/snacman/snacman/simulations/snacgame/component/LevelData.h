#pragma once

#include "Tags.h"
#include "snacman/simulations/snacgame/LevelHelper.h"
#include "../InputConstants.h"

#include <math/Vector.h>
#include <platform/Filesystem.h>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <markovjunior/Grid.h>
#include <math/Matrix.h>

namespace ad {
namespace snacgame {
namespace component {

enum class TileType
{
    Void,
    Path,
    Powerup,
    Portal,
    Spawn,
};

struct Tile
{
    TileType mType;
    int mAllowedMove = gAllowedMovementNone;
    math::Position<2, float> mPos = math::Position<2, float>::Zero();
};

struct PathfindNode
{
    float mReducedCost = std::numeric_limits<float>::max();
    float mCost = std::numeric_limits<float>::max();
    PathfindNode * mPrev = nullptr;
    size_t mIndex = 0;
    math::Position<2, float> mPos;
    bool mPathable = false;
    bool mOpened = false;
};

struct LevelSetupData
{
    // Level grid stored as
    // 0 1 2 ... mColCount - 1 (0th row)
    // mColCount mColCount + 1 ... 2 * mColCount - 1 (1th row)
    // ...
    // mColCount * (mRowCount - 1) ... (mColCount * mRowCount) - 1 (mRowCountth
    // row)
    LevelSetupData(const filesystem::path & aAssetRoot,
              const filesystem::path & aFile,
              const std::vector<math::Size<3, int>> & aAvailableSizes,
              int aSeed) :
        mAssetRoot{aAssetRoot}, mFile{aFile}, mAvailableSizes{aAvailableSizes}, mSeed{aSeed}
    {}

    filesystem::path mAssetRoot;
    filesystem::path mFile;
    std::vector<math::Size<3, int>> mAvailableSizes;
    int mSeed;

};

struct Level
{
    math::Size<3, int> mSize;
    std::vector<Tile> mTiles;
    std::vector<PathfindNode> mNodes;

    std::vector<int> mPortalIndex;
    float mCellSize = 1.f;
};

} // namespace component
} // namespace snacgame
} // namespace ad
