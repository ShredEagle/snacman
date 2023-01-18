#pragma once

#include "entity/EntityManager.h"
#include "entity/Query.h"

#include "../component/Geometry.h"
#include "../component/PlayerLifeCycle.h"

namespace ad {
namespace snacgame {
namespace system {

class PlayerInvulFrame
{
  public:
    PlayerInvulFrame(ent::EntityManager & aWorld) : mPlayer{aWorld}
    {}

    void update(float aDelta);

  private:
    ent::Query<component::PlayerLifeCycle, component::Geometry> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
