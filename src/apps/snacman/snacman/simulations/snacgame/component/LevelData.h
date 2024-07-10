#pragma once

#include "Tags.h"
#include "../InputConstants.h"
#include "../LevelHelper.h"

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mType);
        archive & SERIAL_PARAM(mAllowedMove);
        archive & SERIAL_PARAM(mPos);
    }
};

SNAC_SERIAL_REGISTER(Tile)

struct PathfindNode
{
    float mReducedCost = std::numeric_limits<float>::max();
    float mCost = std::numeric_limits<float>::max();
    PathfindNode * mPrev = nullptr;
    size_t mIndex = 0;
    math::Position<2, float> mPos;
    bool mPathable = false;
    bool mOpened = false;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mReducedCost);
        archive & SERIAL_PARAM(mCost);
        archive & SERIAL_PARAM(mPrev);
        archive & SERIAL_PARAM(mIndex);
        archive & SERIAL_PARAM(mPos);
        archive & SERIAL_PARAM(mPathable);
        archive & SERIAL_PARAM(mOpened);
    }
};

SNAC_SERIAL_REGISTER(PathfindNode)

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mAssetRoot);
        archive & SERIAL_PARAM(mAvailableSizes);
        archive & SERIAL_PARAM(mFile);
        archive & SERIAL_PARAM(mSeed);
    }

};

SNAC_SERIAL_REGISTER(LevelSetupData)

struct Level
{
    math::Size<3, int> mSize;
    std::vector<Tile> mTiles;
    std::vector<PathfindNode> mNodes;

    std::vector<int> mPortalIndex;
    float mCellSize = 1.f;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mSize);
        archive & SERIAL_PARAM(mTiles);
        archive & SERIAL_PARAM(mNodes);
        archive & SERIAL_PARAM(mPortalIndex);
        archive & SERIAL_PARAM(mCellSize);
    }
};

SNAC_SERIAL_REGISTER(Level)

} // namespace component
} // namespace snacgame
} // namespace ad
