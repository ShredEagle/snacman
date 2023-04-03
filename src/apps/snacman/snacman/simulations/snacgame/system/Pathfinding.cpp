#include "Pathfinding.h"

#include "snacman/Profiling.h"

#include "../../../Logging.h"
#include "../typedef.h"
#include "../LevelHelper.h"

#include <cmath>
#include <cstdlib>
#include <queue>

namespace ad {
namespace snacgame {
namespace system {

namespace {

float manhattan(const Pos2 & aA, const Pos2 & aB)
{
    return std::abs(aA.x() - aB.x()) + std::abs(aA.y() - aB.y());
}

} // namespace

void Pathfinding::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    EntHandle level = *mGameContext->mLevel;
    assert(level.isValid() && "Can't pathfind if there is no Level");

    ent::Phase pathfinding;
    component::LevelData levelData = level.get(pathfinding)->get<component::LevelData>();
    const std::vector<component::Tile> & tiles = levelData.mTiles;
    int stride = levelData.mSize.height();
    // TODO: (franz) mNodes should only be updated when dirty
    // For the moment it is dirty if the level is regenerated
    mNodes.clear();
    mNodes.reserve(tiles.size());
    // TODO: (franz) To resolve this mTiles in levelData should
    // probably host all the pathfinding information necessary
    // for this system
    for (size_t i = 0; i < tiles.size(); ++i)
    {
        mNodes.push_back({.mIndex = i,
                          .mPos =
                              math::Position<2, float>{
                                  static_cast<float>(i % stride),
                                  static_cast<float>(i / stride)},
                          .mPathable = tiles.at(i).mType
                                       != component::TileType::Void});
    }

    // Comparator is basically greater on the reduced cost
    // this make the priority queue store the lowest cost
    // node at the top
    auto comparator = [](Node * aLhs, Node * aRhs) {
        return aLhs->mReducedCost > aRhs->mReducedCost;
    };

    using PathfindingQueue =
        std::priority_queue<Node *, std::vector<Node *>, decltype(comparator)>;

    auto visit = [](Node & node, Node * current, float newCost,
                    const Pos2 & targetPos, PathfindingQueue & queue) {
        node.mCost = newCost;
        node.mReducedCost = newCost + manhattan(node.mPos, targetPos);
        node.mPrev = current;

        if (!node.mOpened)
        {
            queue.push(&node);
            node.mOpened = true;
        }
    };

    mPathfinder.each([stride, &tiles, &pathfinding, this, &visit](
                         component::PathToOnGrid & aPathfinder,
                         const component::Geometry & aGeo) {
        EntHandle target = aPathfinder.mEntityTarget;
        assert(target.isValid()
               && "Trying to pathfind to an obsolete handle");
        assert(target.get(pathfinding)->has<component::Geometry>()
               && "Trying to pathfind to an entity with no geometry");
        const Pos2 & targetPos = getLevelPosition(
            target.get(pathfinding)->get<component::Geometry>().mPosition);

        // Since priority do not have a clear() method we need to reinstantiate
        // a priority queue for all pathfinder
        PathfindingQueue openedNode;

        // We need a copy of mNodes to make the calculation in place
        std::vector<Node> localNodes = mNodes;

        if (tiles.at((size_t) targetPos.x() + (size_t) targetPos.y() * stride)
                .mType
            == component::TileType::Void)
        {
            SELOG(info)("Pathfinding to a entity not on a Path");
            // TODO: (franz) handle the case where the target is not
            // in a pathable position
            return;
        }

        // Setup opened node with the start position and the node
        // where the pathfinder is residing so that when we try to path
        // so that the residing node is a possible destination

        Pos2 startPos = getLevelPosition(aGeo.mPosition);
        Node start{
            .mReducedCost = manhattan(aGeo.mPosition.xy(), targetPos),
            .mCost = 0,
            .mIndex = (size_t) startPos.x() + (size_t) startPos.y() * stride,
            .mPos = aGeo.mPosition.xy(),
            .mOpened = true,
        };
        openedNode.push(&start);

        Node & startTile = localNodes.at(start.mIndex);
        startTile.mCost = (aGeo.mPosition.xy() - startTile.mPos).getNorm();
        startTile.mPrev = &start;
        startTile.mReducedCost =
            startTile.mCost + manhattan(startTile.mPos, targetPos);

        // If the start tile as the cost of zero it means
        // that the pathfinder is perfectly on the tile we must not
        // path to the tile otherwise we will be stuck
        if (startTile.mCost != 0.f)
        {
            openedNode.push(&startTile);
            startTile.mOpened = true;
        }

        Node * closestNode = nullptr;

        while (openedNode.size() > 0)
        {
            // When we want to find a new node to explore we want to
            // find the node with the lowest reduced cost
            // (distance from start + heuristic)
            // However when we think about revisiting a node
            // we want to allow revisit if the new cost (distance from start) is lower 
            // than the previous visit
            Node * current = openedNode.top();
            openedNode.pop();
            current->mOpened = false;

            if (current->mPos == targetPos)
            {
                closestNode = current;
                break;
            }

            for (int x = -1; x <= 1; x += 2)
            {
                Node & visitedNode =
                    localNodes.at(static_cast<int>(current->mIndex) + x);
                float distance = (visitedNode.mPos - current->mPos).getNorm();
                float newCost = current->mCost + distance;

                // the distance <= 1.f guarantees that the pathfinder 
                // cannot cut corners because the next node in it's path cannot
                // be more than 1 cell away
                if (visitedNode.mPathable && newCost < visitedNode.mCost
                    && distance <= 1.f)
                {
                    visit(visitedNode, current, newCost, targetPos, openedNode);
                }
            }
            for (int y = -1; y <= 1; y += 2)
            {
                Node & visitedNode =
                    localNodes.at((int) current->mIndex + y * stride);
                float distance = (visitedNode.mPos - current->mPos).getNorm();
                float newCost = current->mCost + distance;
                if (visitedNode.mPathable && newCost < visitedNode.mCost
                    && distance <= 1.f)
                {
                    visit(visitedNode, current, newCost, targetPos, openedNode);
                }
            }
        }

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
