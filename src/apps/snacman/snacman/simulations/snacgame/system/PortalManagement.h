#pragma once

#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/PlayerModel.h"
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

    void preGraphUpdate();
    void postGraphUpdate();

private:
    GameContext * mGameContext;
    ent::Query<component::GlobalPose, component::Collision, component::PlayerPortalData, component::Geometry, component::PlayerModel> mPlayer;
    ent::Query<component::Portal, component::GlobalPose, component::Geometry> mPortals;
};

} // namespace system
} // namespace snacgame
} // namespace ad
