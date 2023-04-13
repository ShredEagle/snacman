#pragma once

#include "../GameContext.h"

#include "../component/LevelData.h"
#include "../component/SceneNode.h"

#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class LevelCreator
{
public:
    LevelCreator(GameContext & aGameContext) :
        mGameContext{&aGameContext}, mCreatable{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::LevelData,
               component::LevelToCreate,
               component::SceneNode>
        mCreatable;
};

} // namespace system
} // namespace snacgame
} // namespace ad
