#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include "Tags.h"
#include "../InputConstants.h"
#include "../LevelHelper.h"

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
    TileType mType = TileType::Void;
    int mAllowedMove = gAllowedMovementNone;
    math::Position<2, float> mPos = math::Position<2, float>::Zero();

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mAllowedMove));
        aWitness.witness(NVP(mPos));
    }
};

REFLEXION_REGISTER(Tile)

struct PathfindNode
{
    float mReducedCost = std::numeric_limits<float>::max();
    float mCost = std::numeric_limits<float>::max();
    PathfindNode * mPrev = nullptr;
    size_t mIndex = 0;
    math::Position<2, float> mPos;
    bool mPathable = false;
    bool mOpened = false;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mReducedCost));
        aWitness.witness(NVP(mCost));
        aWitness.witness(NVP(mIndex));
        aWitness.witness(NVP(mPos));
        aWitness.witness(NVP(mPathable));
        aWitness.witness(NVP(mOpened));
    }
};

REFLEXION_REGISTER(PathfindNode)

struct LevelSetupData
{
    LevelSetupData()
    {}

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mAssetRoot));
        aWitness.witness(NVP(mFile));
        aWitness.witness(NVP(mAvailableSizes));
        aWitness.witness(NVP(mSeed));
    }
};

REFLEXION_REGISTER(LevelSetupData)

struct Level
{
    math::Size<3, int> mSize;
    std::vector<Tile> mTiles;
    std::vector<PathfindNode> mNodes;

    std::vector<int> mPortalIndex;
    float mCellSize = 1.f;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mSize));
        aWitness.witness(NVP(mTiles));
        aWitness.witness(NVP(mNodes));
        aWitness.witness(NVP(mPortalIndex));
        aWitness.witness(NVP(mCellSize));
    }
};

REFLEXION_REGISTER(Level)

} // namespace component
} // namespace snacgame
} // namespace ad
