#pragma once

#include "../component/SceneNode.h"
#include "../GameContext.h"
#include "../component/LevelData.h"

#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class LevelCreator
{
public:
    LevelCreator(ent::EntityManager * aWorld) :
        mCreatable{*aWorld}
    {}

    void update(GameContext & aContext);

private:
    ent::Query<component::LevelData, component::LevelToCreate, component::SceneNode> mCreatable;
};

} // namespace system
} // namespace snacgame
} // namespace ad
