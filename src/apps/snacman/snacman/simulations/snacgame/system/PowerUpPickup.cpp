#include "PowerUpPickup.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/PlayerPowerUp.h"


#include "../typedef.h"

#include <math/Transformations.h>
#include <math/Box.h>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpPickup::update()
{
    Phase powerup;
    mPlayers.each([&](EntHandle aPlayer, component::Geometry & aPlayerGeo,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol) {
        const math::Vec<3,float> & worldPos = aPlayerGeo.mPosition.as<math::Vec>();
        math::Box<float> playerHitbox = aPlayerCol.mHitbox;
        math::Position<4, float> transformedPos =  math::homogeneous::makePosition(playerHitbox.mPosition) * math::trans3d::translate(worldPos);
        playerHitbox.mPosition = transformedPos.xyz();
        mPowerups.each([&](ent::Handle<ent::Entity> aHandle,
                        component::PowerUp & aPowerup,
                        const component::Geometry & aPowerupGeo,
                        const component::Collision & aPowerupCol) {
            const math::Vec<3,float> & worldPos = aPowerupGeo.mPosition.as<math::Vec>();
            math::Box<float> powerupHitbox = aPowerupCol.mHitbox;
            math::Position<4, float> transformedPos =  math::homogeneous::makePosition(powerupHitbox.mPosition) * math::trans3d::translate(worldPos);
            powerupHitbox.mPosition = transformedPos.xyz();

            if (!aPowerup.mPickedUp && component::collideWithSat(powerupHitbox, playerHitbox))
            {
                Entity playerEnt = *aPlayer.get(powerup);
                if (!playerEnt.has<component::PlayerPowerUp>())
                {
                    EntHandle aPlayerPowerup = createPlayerPowerUp(*mGameContext);
                    playerEnt.add(component::PlayerPowerUp{.mPowerUp = aPlayerPowerup, .mType = aPowerup.mType});
                    insertEntityInScene(aPlayerPowerup, aPlayer);
                    aPowerup.mPickedUp = true;
                    aHandle.get(powerup)->erase();
                }
            }
        });
    });
}
}
} // namespace snacgame
} // namespace ad
