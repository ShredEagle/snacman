#pragma once

#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/PlayerPortalData.h"
#include "../GameContext.h"
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
    PortalManagement(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mPlayer{mGameContext->mWorld},
        mPortals{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::Geometry, component::PlayerMoveState, component::Collision, component::PlayerPortalData> mPlayer;
    ent::Query<component::Geometry, component::Portal, component::Collision> mPortals;
};

} // namespace system
} // namespace snacgame
} // namespace ad
