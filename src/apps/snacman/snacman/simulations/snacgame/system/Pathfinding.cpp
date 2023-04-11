#include "Pathfinding.h"

#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/component/LevelData.h"

#include "../../../Logging.h"
#include "../typedef.h"
#include "../LevelHelper.h"

#include <cmath>
#include <cstdlib>
#include <queue>

namespace ad {
namespace snacgame {
namespace system {

void Pathfinding::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    EntHandle level = *mGameContext->mLevel;
    assert(level.isValid() && "Can't pathfind if there is no Level");

    ent::Phase pathfinding;
    component::LevelData levelData = level.get(pathfinding)->get<component::LevelData>();
    const std::vector<component::Tile> & tiles = levelData.mTiles;
    const std::vector<component::PathfindNode> & nodes = levelData.mNodes;
    int stride = levelData.mSize.height();

    mPathfinder.each([stride, &tiles, &pathfinding, nodes](
                         component::PathToOnGrid & aPathfinder,
                         const component::Geometry & aGeo) {
        EntHandle target = aPathfinder.mEntityTarget;
        assert(target.isValid()
               && "Trying to pathfind to an obsolete handle");
        assert(target.get(pathfinding)->has<component::Geometry>()
               && "Trying to pathfind to an entity with no geometry");
        const Pos2 & targetPos = getLevelPosition(
            target.get(pathfinding)->get<component::Geometry>().mPosition.xy());



        if (tiles.at((size_t) targetPos.x() + (size_t) targetPos.y() * stride)
                .mType
            == component::TileType::Void)
        {
            SELOG(info)("Pathfinding to a entity not on a Path");
            // TODO: (franz) handle the case where the target is not
            // in a pathable position
            return;
        }

        // We need a copy of nodes to make the calculation in place
        // so nodes is captured by value
        std::vector<component::PathfindNode> localNodes = nodes;
        component::PathfindNode * closestNode = pathfind(aGeo.mPosition.xy(), targetPos, localNodes, stride);

        // There is two possible states
        // 1. The target and pathfinder are perfectly on the same tile -> there
        //    is only one node in the path
        // 2. All other case -> there is at least two node in the path and we
        //    want the second node in the path to be the target
        while (closestNode->mPrev != nullptr
               && closestNode->mPrev->mPrev != nullptr)
        {
            closestNode = closestNode->mPrev;
        }

        aPathfinder.mCurrentTarget = closestNode->mPos;
        aPathfinder.mTargetFound = true;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
