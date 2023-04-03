#pragma once

#include "snacman/simulations/snacgame/GameContext.h"
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
        mGameContext{&aGameContext},
        mPathfinder{mGameContext->mWorld}
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
        bool mOpened = false;
    };

    GameContext * mGameContext;
    ent::Query<component::PathToOnGrid, component::Geometry> mPathfinder;
    std::vector<Node> mNodes;
};

} // namespace component
} // namespace snacgame
} // namespace ad