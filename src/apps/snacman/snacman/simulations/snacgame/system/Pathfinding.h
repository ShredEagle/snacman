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

    void update();

private:
    struct Node
    {
        float mReducedCost = std::numeric_limits<float>::max();
        float mCost = std::numeric_limits<float>::max();
        Node * mPrev = nullptr;
        size_t mIndex;
        math::Position<2, float> mPos;
        bool mPathable = false;
        bool opened = false;
    };

    ent::Query<component::PathToOnGrid, component::Geometry> mPathfinder;
    ent::Handle<ent::Entity> mLevel;
    std::vector<Node> mNodes;
};

} // namespace component
} // namespace snacgame
} // namespace ad
