#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"
#include "snacman/Input.h"

#include "../ActionKeyMapper.h"
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
        mLevel{aLevel}
    {}

    void update(const snac::Input & aInput);

private:
    ent::Query<component::Controller,
               component::Geometry,
               component::PlayerMoveState>
        mPlayer;
    ActionKeyMapper<gGamepadDefaultMapping> mGamepadMapping;
    ActionKeyMapper<gKeyboardDefaultMapping> mKeyboardMapping;
    ent::Handle<ent::Entity> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
