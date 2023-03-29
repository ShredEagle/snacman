#pragma once

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
    Pathfinding(ent::EntityManager & aWorld, ent::Handle<ent::Entity> aLevel) :
        mPathfinder{aWorld}, mLevel{aLevel}
    {}

    void update(float aDelta);

private:
    struct Node
    {
        unsigned int mCost = std::numeric_limits<unsigned int>::max();
        Node * mPrev = nullptr;
        size_t mIndex;
        math::Position<2, int> mPos;
        bool mVisited = false;
    };

    ent::Query<component::PathToOnGrid, component::Geometry> mPathfinder;
    ent::Handle<ent::Entity> mLevel;
    std::vector<Node> mNodes;
};

} // namespace component
} // namespace snacgame
} // namespace ad
