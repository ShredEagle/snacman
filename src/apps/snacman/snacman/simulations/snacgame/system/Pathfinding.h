#pragma once

#include "../GameParameters.h"
#include "../LevelHelper.h"

#include "../component/LevelData.h"

#include <entity/Entity.h>
#include <entity/Query.h>
#include <limits>
#include <queue>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Geometry;
struct PathToOnGrid;
struct Level;
}
namespace system {

class Pathfinding
{
public:
    Pathfinding(GameContext & aGameContext);

    void update(component::Level & aLevelData);

private:
    GameContext * mGameContext;
    ent::Query<component::PathToOnGrid,
               component::Geometry>
        mPathfinder;
};

inline component::PathfindNode createStartPathfindNode(const math::Position<2, float> & aSource,
         const math::Position<2, float> & aTarget,
         size_t stride
)
{
    math::Position<2, float> startPos = getLevelPosition(aSource);
    component::PathfindNode start{
        .mReducedCost = manhattan(aSource, aTarget),
        .mCost = 0,
        .mIndex = (size_t) startPos.x() + (size_t) startPos.y() * stride,
        .mPos = aSource,
        .mOpened = true,
    };

    return start;
}

inline component::PathfindNode
pathfind(component::PathfindNode & aStartNode,
         const math::Position<2, float> & aTarget,
         std::vector<component::PathfindNode> & aNodes,
         size_t stride)
{
    // Comparator is basically greater on the reduced cost
    // this make the priority queue store the lowest cost
    // node at the top
    constexpr auto comparator = [](component::PathfindNode * aLhs,
                                   component::PathfindNode * aRhs) {
        return aLhs->mReducedCost > aRhs->mReducedCost;
    };

    using PathfindingQueue =
        std::priority_queue<component::PathfindNode *,
                            std::vector<component::PathfindNode *>,
                            decltype(comparator)>;

    constexpr auto visitNode =
        [](component::PathfindNode & node, component::PathfindNode * current,
           float newCost, const math::Position<2, float> & targetPos,
           PathfindingQueue & queue) {
            node.mCost = newCost;
            node.mReducedCost = newCost + manhattan(node.mPos, targetPos);
            node.mPrev = current;

            if (!node.mOpened)
            {
                queue.push(&node);
                node.mOpened = true;
            }
        };

    // setup opened node with the start position and the node
    // where the pathfinder is residing so that when we try to path
    // so that the residing node is a possible destination

    // Since priority do not have a clear() method we need to reinstantiate
    // a priority queue for all pathfinder
    PathfindingQueue openedNode(comparator);
    openedNode.push(&aStartNode);

    component::PathfindNode & startTile = aNodes.at(aStartNode.mIndex);
    startTile.mCost = (aStartNode.mPos - startTile.mPos).getNorm();
    startTile.mPrev = &aStartNode;
    startTile.mReducedCost =
        startTile.mCost + manhattan(startTile.mPos, aTarget);

    // If the start tile as the cost of zero it means
    // that the pathfinder is perfectly on the tile we must not
    // path to the tile otherwise we will be stuck
    if (startTile.mCost != 0.f)
    {
        openedNode.push(&startTile);
        startTile.mOpened = true;
    }

    component::PathfindNode * closestNode = nullptr;

    while (openedNode.size() > 0)
    {
        // When we want to find a new node to explore we want to
        // find the node with the lowest reduced cost
        // (distance from start + heuristic)
        // However when we think about revisiting a node
        // we want to allow revisit if the new cost (distance from start) is
        // lower than the previous visit
        component::PathfindNode * current = openedNode.top();
        openedNode.pop();
        current->mOpened = false;

        if (current->mPos.equalsWithinTolerance(aTarget, gCellSize / 2))
        {
            closestNode = current;
            break;
        }

        for (math::Vec<2, int> offset : gDirections)
        {
            component::PathfindNode & visitedNode =
                aNodes.at(static_cast<size_t>((int) current->mIndex + offset.x()
                                              + offset.y() * (int) stride));
            float distance = (visitedNode.mPos - current->mPos).getNorm();
            float newCost = current->mCost + distance;

            // the distance <= 1.f guarantees that the pathfinder
            // cannot cut corners because the next node in it's path cannot
            // be more than 1 cell away
            if (visitedNode.mPathable && newCost < visitedNode.mCost
                && distance <= 1.f)
            {
                visitNode(visitedNode, current, newCost, aTarget, openedNode);
            }
        }
    }

    return *closestNode;
}

} // namespace system
} // namespace snacgame
} // namespace ad
