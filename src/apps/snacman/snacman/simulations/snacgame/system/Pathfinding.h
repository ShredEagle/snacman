#pragma once

#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/LevelHelper.h"

#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/PathToOnGrid.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <limits>

namespace ad {
namespace snacgame {
namespace system {

class Pathfinding
{
public:
    Pathfinding(GameContext & aGameContext) :
        mGameContext{&aGameContext}, mPathfinder{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::PathToOnGrid, component::Geometry> mPathfinder;
};

inline float manhattan(const math::Position<2, float> & aA,
                       const math::Position<2, float> & aB)
{
    return std::abs(aA.x() - aB.x()) + std::abs(aA.y() - aB.y());
}

inline component::PathfindNode *
pathfind(const math::Position<2, float> & aSource,
         const math::Position<2, float> & aTarget,
         std::vector<component::PathfindNode> & aNodes,
         int stride)
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

    // Setup opened node with the start position and the node
    // where the pathfinder is residing so that when we try to path
    // so that the residing node is a possible destination

    math::Position<2, float> startPos = getLevelPosition(aSource);

    // Since priority do not have a clear() method we need to reinstantiate
    // a priority queue for all pathfinder
    PathfindingQueue openedNode(comparator);
    component::PathfindNode start{
        .mReducedCost = manhattan(aSource, aTarget),
        .mCost = 0,
        .mIndex = (size_t) startPos.x() + (size_t) startPos.y() * stride,
        .mPos = aSource,
        .mOpened = true,
    };
    openedNode.push(&start);

    component::PathfindNode & startTile = aNodes.at(start.mIndex);
    startTile.mCost = (aSource - startTile.mPos).getNorm();
    startTile.mPrev = &start;
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

        if (current->mPos == aTarget)
        {
            closestNode = current;
            break;
        }

        constexpr std::array<Pos2, 4> direction{
            Pos2{1.f, 0.f}, Pos2{-1.f, 0.f}, Pos2{0.f, 1.f}, Pos2{0.f, -1.f}};
        for (Pos2 offset : direction)
        {
            component::PathfindNode & visitedNode = aNodes.at(
                static_cast<int>(current->mIndex) + static_cast<int>(offset.x())
                + static_cast<int>(offset.y()) * stride);
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

    return closestNode;
}

} // namespace system
} // namespace snacgame
} // namespace ad
