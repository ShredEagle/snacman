#pragma once

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
    ent::Query<component::LevelData, component::LevelToCreate> mCreatable;
};

} // namespace system
} // namespace snacgame
} // namespace ad
