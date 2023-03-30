#include "Pathfinding.h"

#include "snacman/Profiling.h"

#include "../../../Logging.h"
#include "../typedef.h"

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

Pos2 getLevelPosition(const Pos3 & aPos)
{
    return Pos2{std::floor(aPos.x() + 0.5f), std::floor(aPos.y() + 0.5f)};
}
} // namespace

void Pathfinding::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    // Should be in an Entity
    ent::Phase init;
    component::LevelData data = mLevel.get(init)->get<component::LevelData>();
    mNodes.clear();
    mNodes.reserve(data.mTiles.size());
    for (size_t i = 0; i < data.mTiles.size(); ++i)
    {
        mNodes.push_back({.mIndex = i,
                          .mPos =
                              math::Position<2, float>{
                                  static_cast<float>(i % data.mSize.height()),
                                  static_cast<float>(i / data.mSize.height())},
                          .mPathable = data.mTiles.at(i).mType
                                       != component::TileType::Void});
    }
    // Should be in an Entity

    ent::Phase pathfinding;
    const component::LevelData & levelData =
        mLevel.get(pathfinding)->get<component::LevelData>();
    const std::vector<component::Tile> & tiles = levelData.mTiles;
    int stride = levelData.mSize.height();

    // Compartor is basically greater on the cost
    // this make the priority queue store the lowest cost
    // node at the top
    auto comparator = [](Node * aLhs, Node * aRhs) {
        return aLhs->mReducedCost > aRhs->mReducedCost;
    };

    mPathfinder.each([stride, &tiles, &pathfinding, this](
                         component::PathToOnGrid & aPathfinder,
                         component::Geometry & aGeo) {
        EntHandle target = aPathfinder.mEntityTarget;
        assert(target.get(pathfinding)->has<component::Geometry>()
               && "Trying to pathfind to an entity with no geometry");
        const Pos2 & targetPos = getLevelPosition(
            target.get(pathfinding)->get<component::Geometry>().mPosition);

        std::priority_queue<Node *, std::vector<Node *>, decltype(comparator)>
            openedNode;

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

        Pos2 startPos = getLevelPosition(aGeo.mPosition);
        Node start{
            .mIndex = (size_t) startPos.x() + (size_t) startPos.y() * stride,
            .mPos = aGeo.mPosition.xy(),
        };
        start.mCost = 0;
        start.mReducedCost = manhattan(aGeo.mPosition.xy(), targetPos);
        openedNode.push(&start);
        start.opened = true;
        Node & startTile = localNodes.at(start.mIndex);
        startTile.mCost = (aGeo.mPosition.xy() - startTile.mPos).getNorm();
        startTile.mPrev = &start;
        startTile.mReducedCost =
            startTile.mCost + manhattan(startTile.mPos, targetPos);

        if (startTile.mCost != 0.f)
        {
            openedNode.push(&startTile);
            startTile.opened = true;
        }

        Node * closestNode = nullptr;

        while (openedNode.size() > 0)
        {
            Node * current = openedNode.top();
            openedNode.pop();
            current->opened = false;

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
                if (visitedNode.mPathable && newCost < visitedNode.mCost
                    && distance <= 1.f)
                {
                    // All node have a 1 cost right now
                    // because in manhattan distance all tile have a cost
                    // of 1 (maybe we could discard this)
                    visitedNode.mCost = newCost;
                    visitedNode.mReducedCost =
                        newCost
                        + manhattan(current->mPos + Vec2{(float) x, 0.f},
                                    targetPos);
                    visitedNode.mPrev = current;

                    if (!visitedNode.opened)
                    {
                        openedNode.push(&visitedNode);
                        visitedNode.opened = true;
                    }
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
                    // All node have a 1 cost right now
                    // because in manhattan distance all tile have a cost
                    // of 1 (maybe we could discard this)
                    visitedNode.mCost = newCost;
                    visitedNode.mReducedCost =
                        newCost
                        + manhattan(current->mPos + Vec2{(float) y, 0.f},
                                    targetPos);
                    visitedNode.mPrev = current;
                    if (!visitedNode.opened)
                    {
                        openedNode.push(&visitedNode);
                        visitedNode.opened = true;
                    }
                }
            }
        }

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
