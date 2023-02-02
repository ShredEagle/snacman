#pragma once

#include "../component/Geometry.h"
#include "../component/PlayerLifeCycle.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

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
