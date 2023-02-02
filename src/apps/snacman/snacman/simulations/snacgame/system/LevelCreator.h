#pragma once

#include "../component/LevelData.h"

#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class LevelCreator
{
public:
    LevelCreator(ent::EntityManager * aWorld) :
        mWorld{aWorld}, mCreatable{*aWorld}
    {}

    void update();

private:
    ent::EntityManager * mWorld;
    ent::Query<component::LevelData, component::LevelToCreate> mCreatable;
};

} // namespace system
} // namespace snacgame
} // namespace ad
