#pragma once

#include "snacman/simulations/snacgame/component/PoseScreenSpace.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/system/MovementIntegration.h"
#include "../GameContext.h"
#include "../component/Text.h"

#include <cstdio>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class FallingPlayersSystem
{
public:
    FallingPlayersSystem(GameContext & aGameContext) : 
        mFallingPlayers{aGameContext.mWorld}
    {}

    void update()
    {
        //Check player collision to the ground
        ent::Phase removeSpeed;
        mFallingPlayers.each([&removeSpeed](
                          ad::ent::Handle<ad::ent::Entity> aHandle, component::Geometry & aGeometry,
                          const component::Gravity & aGravity) {
            if (aGeometry.mPosition.z() <= aGravity.mFloorHeight)
            {
                aGeometry.mPosition.z() = aGravity.mFloorHeight;
                aHandle.get(removeSpeed)->remove<component::Speed>();
            }
        });
    }

private:
    ent::Query<component::Gravity, component::Geometry> mFallingPlayers;
};

}
}
}
