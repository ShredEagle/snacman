#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/LevelData.h"

#include "../InputCommandConverter.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/PlayerMoveState.h"

namespace ad {
namespace snacgame {
namespace system {

class DeterminePlayerAction
{
public:
    DeterminePlayerAction(ent::EntityManager & aWorld,
                          ent::Handle<ent::Entity> aLevel) :
        mPlayer{aWorld},
        mLevel{aWorld}
    {}

    void update();

private:
    ent::Query<component::Controller,
               component::Geometry,
               component::PlayerMoveState>
        mPlayer;

    ent::Query<component::LevelData, component::LevelCreated> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
