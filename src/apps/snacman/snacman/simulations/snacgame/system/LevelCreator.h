#pragma once

#include "snacman/simulations/snacgame/component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/SceneNode.h"
#include "../GameContext.h"

#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class LevelCreator
{
public:
    LevelCreator(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mCreatable{mGameContext->mWorld},
        mEntities{mGameContext->mWorld},
        mPortals{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::LevelData,
               component::LevelToCreate,
               component::SceneNode>
        mCreatable;
    ent::Query<component::LevelEntity, component::SceneNode> mEntities;
    ent::Query<component::Portal, component::Geometry> mPortals;
};

} // namespace system
} // namespace snacgame
} // namespace ad
