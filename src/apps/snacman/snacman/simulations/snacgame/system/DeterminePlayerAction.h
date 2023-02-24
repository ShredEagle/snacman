#pragma once

#include "snacman/Input.h"

#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/PlayerMoveState.h"
#include "../InputCommandConverter.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class DeterminePlayerAction
{
public:
    DeterminePlayerAction(ent::EntityManager & aWorld,
                          ent::Handle<ent::Entity> aLevel) :
        mPlayer{aWorld}, mLevel{aWorld}, mPaths{aWorld}
    {}

    void update();

private:
    ent::Query<component::Controller,
               component::Geometry,
               component::PlayerMoveState>
        mPlayer;

    ent::Query<component::LevelData, component::LevelCreated> mLevel;
    //Debug
    ent::Query<component::LevelEntity, component::Geometry> mPaths;
};

} // namespace system
} // namespace snacgame
} // namespace ad
