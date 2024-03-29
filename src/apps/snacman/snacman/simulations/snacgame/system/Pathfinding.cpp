#include "Pathfinding.h"

#include "../typedef.h"
#include "../LevelHelper.h"
#include "../GameContext.h"

#include "../component/Geometry.h"
#include "../component/PathToOnGrid.h"
#include "../component/LevelData.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include <snacman/Logging.h>
#include <snacman/Profiling.h>

#include <cmath>
#include <cstdlib>
#include <queue>

namespace ad {
namespace snacgame {
namespace system {

Pathfinding::Pathfinding(GameContext & aGameContext) :
    mGameContext{&aGameContext}, mPathfinder{mGameContext->mWorld}
{}

void Pathfinding::update(component::Level & aLevelData)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase pathfinding;
    const std::vector<component::Tile> & tiles = aLevelData.mTiles;
    const std::vector<component::PathfindNode> & nodes = aLevelData.mNodes;
    int stride = aLevelData.mSize.height();

    mPathfinder.each([stride, &tiles, &pathfinding, &nodes](
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
        // We need to create the startNode locally because it can be pointed to
        // by the resulting pathfind node and would be unstacked if it's created inside
        // pathfind
        component::PathfindNode startNode = createStartPathfindNode(aGeo.mPosition.xy(), targetPos, stride);
        component::PathfindNode closestNode = pathfind(startNode, targetPos,  localNodes, stride);

        // There is two possible states
        // 1. The target and pathfinder are perfectly on the same tile -> there
        //    is only one node in the path
        // 2. All other case -> there is at least two node in the path and we
        //    want the second node in the path to be the target
        while (closestNode.mPrev != nullptr
               && closestNode.mPrev->mPrev != nullptr)
        {
            closestNode = *closestNode.mPrev;
        }

        aPathfinder.mCurrentTarget = closestNode.mPos;
        aPathfinder.mTargetFound = true;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
