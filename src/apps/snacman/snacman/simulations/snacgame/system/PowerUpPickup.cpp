#include "PowerUpPickup.h"

#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/component/PlayerPowerUp.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../typedef.h"

#include <math/Box.h>
#include <math/Transformations.h>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpPickup::update()
{
    Phase powerup;
    mPlayers.each([&](EntHandle aPlayer, component::Geometry & aPlayerGeo,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol) {
        const Vec3 & worldPos = aPlayerGeo.mPosition.as<math::Vec>();
        Box_f playerHitbox = aPlayerCol.mHitbox;
        Pos4 transformedPos =
            math::homogeneous::makePosition(playerHitbox.mPosition)
            * math::trans3d::translate(worldPos);
        playerHitbox.mPosition = transformedPos.xyz();

        Entity playerEnt = *aPlayer.get(powerup);
        if (!playerEnt.has<component::PlayerPowerUp>())
        {
            mPowerups.each([&](ent::Handle<ent::Entity> aHandle,
                               component::PowerUp & aPowerup,
                               const component::Geometry & aPowerupGeo,
                               const component::Collision & aPowerupCol) {
                const Vec3 & worldPos = aPowerupGeo.mPosition.as<math::Vec>();
                Box_f powerupHitbox = aPowerupCol.mHitbox;
                Pos4 transformedPos =
                    math::homogeneous::makePosition(powerupHitbox.mPosition)
                    * math::trans3d::translate(worldPos);
                powerupHitbox.mPosition = transformedPos.xyz();

                if (!aPowerup.mPickedUp
                    && component::collideWithSat(powerupHitbox, playerHitbox))
                {
                    EntHandle aPlayerPowerup =
                        createPlayerPowerUp(*mGameContext);
                    playerEnt.add(component::PlayerPowerUp{
                        .mPowerUp = aPlayerPowerup, .mType = aPowerup.mType});
                    insertEntityInScene(aPlayerPowerup, aPlayer);
                    aPowerup.mPickedUp = true;
                    aHandle.get(powerup)->erase();
                }
            });
        }
    });

    mPowUpPlayers.each([](EntHandle aPlayer, const component::Geometry & aPlayerGeo,
                          const component::PlayerPowerUp & aPowerUp, const component::Controller & aController) {
        if (aController.mCommandQuery == gPlayerUsePowerup)
        {
            //Drop power up in front of player
        }
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
