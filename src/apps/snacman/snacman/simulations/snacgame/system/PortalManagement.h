#pragma once

#include "../component/LevelTags.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/PlayerMoveState.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class PortalManagement
{
public:
    PortalManagement(ent::EntityManager & aWorld, ent::Handle<ent::Entity> mLevel) :
        mPlayer{aWorld},
        mPortals{aWorld},
        mLevel{mLevel}
    {}

    void update();

private:
    ent::Query<component::Geometry, component::PlayerMoveState> mPlayer;
    ent::Query<component::Geometry, component::Portal> mPortals;
    ent::Handle<ent::Entity> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
