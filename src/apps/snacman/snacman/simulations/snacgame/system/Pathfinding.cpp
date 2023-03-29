#include "Pathfinding.h"

#include "../../../Logging.h"
#include "../typedef.h"

#include <cmath>
#include <cstdlib>
#include <queue>

namespace ad {
namespace snacgame {
namespace system {

namespace {
int manhattan(Pos2_i aA, Pos2_i aB)
{
    return std::abs(aA.x() - aB.x()) + std::abs(aA.y() - aB.y());
}

Pos2_i getLevelPosition(Pos3 aPos)
{
    return Pos2_i{static_cast<int>(aPos.x() + 0.5f),
                  static_cast<int>(aPos.y() + 0.5f)};
}
} // namespace

void Pathfinding::update(float aDelta)
{
    // Should be in an Entity
    ent::Phase init;
    component::LevelData data =
        mLevel.get(init)->get<component::LevelData>();
    mNodes.clear();
    mNodes.reserve(data.mTiles.size());
    for (size_t i = 0; i < data.mTiles.size(); ++i)
    {
        mNodes.push_back({.mIndex = i,
                        .mPos = math::Position<2, int>{
                            static_cast<int>(i % data.mSize.height()),
                            static_cast<int>(i / data.mSize.height())},
                            .mVisited = data.mTiles.at(i).mType == component::TileType::Void});
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
        return aLhs->mCost > aRhs->mCost;
    };

    mPathfinder.each([aDelta, stride, &tiles, &pathfinding, this](
                         component::PathToOnGrid & aPathfinder,
                         component::Geometry & aGeo) {

        EntHandle target = aPathfinder.mEntityTarget;
        assert(target.get(pathfinding)->has<component::Geometry>()
               && "Trying to pathfind to an entity with no geometry");
        const Pos2_i & targetPos = getLevelPosition(
            target.get(pathfinding)->get<component::Geometry>().mPosition);

        const Pos2_i pathfinderPos = getLevelPosition(aGeo.mPosition);

        std::priority_queue<Node *, std::vector<Node *>, decltype(comparator)>
            openedNode;

        // We need a copy of mNodes to make the calculation in place
        std::vector<Node> localNodes = mNodes;

        if (tiles.at(targetPos.x() + targetPos.y() * stride).mType
            == component::TileType::Void)
        {
            SELOG(info)("Pathfinding to a entity not on a Path");
            // TODO: (franz) handle the case where the target is not
            // in a pathable position
            return;
        }

        Node & current =
            localNodes.at(pathfinderPos.x() + pathfinderPos.y() * stride);
        current.mCost = 0;

        openedNode.push(&current);

        Node * closestNode = nullptr;

        while (openedNode.size() > 0)
        {
            Node * current = openedNode.top();
            openedNode.pop();

            if (current->mPos == targetPos)
            {
                closestNode = current;
                break;
            }

            current->mVisited = true;

            for (int x = -1; x <= 1; x += 2)
            {
                Node & visitedNode =
                    localNodes.at((int) current->mIndex + x);
                if (!visitedNode.mVisited)
                {
                    // All node have a 1 cost right now
                    // because in manhattan distance all tile have a cost
                    // of 1 (maybe we could discard this)
                    visitedNode.mCost =
                        current->mCost + 1
                        + manhattan(current->mPos + Vec2_i{x, 0},
                                    targetPos);
                    visitedNode.mPrev = current;
                    openedNode.push(&visitedNode);
                }
            }
            for (int y = -1; y <= 1; y += 2)
            {
                Node & visitedNode =
                    localNodes.at((int) current->mIndex + y * stride);
                if (!visitedNode.mVisited)
                {
                    visitedNode.mVisited = true;
                    // All node have a 1 cost right now
                    // because in manhattan distance all tile have a cost
                    // of 1 (maybe we could discard this)
                    visitedNode.mCost =
                        current->mCost + 1
                        + manhattan(current->mPos + Vec2_i{0, y},
                                    targetPos);
                    visitedNode.mPrev = current;
                    openedNode.push(&visitedNode);
                }
            }
        }

        while (closestNode->mPrev != nullptr && closestNode->mPrev->mPrev != nullptr)
        {
            closestNode = closestNode->mPrev;
        }

        aPathfinder.mCurrentTarget = closestNode->mPos;
        Vec3 displacement = Vec3{ 
            static_cast<float>(aPathfinder.mCurrentTarget.x() - pathfinderPos.x()),
            static_cast<float>(aPathfinder.mCurrentTarget.y() - pathfinderPos.y()), 0.f} * aDelta;
        aGeo.mPosition = aGeo.mPosition + displacement;
    });
}

} // namespace component
} // namespace snacgame
} // namespace ad
