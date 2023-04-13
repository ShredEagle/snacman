#pragma once

#include "../GameContext.h"
#include "../InputCommandConverter.h"

#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/AllowedMovement.h"

#include <snacman/Input.h>

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class AllowMovement
{
public:
    AllowMovement(GameContext & aGameContext) :
        mGameContext{&aGameContext},
        mMover{mGameContext->mWorld}
    {}

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::Geometry,
               component::AllowedMovement>
        mMover;
};

} // namespace system
} // namespace snacgame
} // namespace ad
