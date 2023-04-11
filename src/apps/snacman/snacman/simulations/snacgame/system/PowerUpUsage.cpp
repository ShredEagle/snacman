#include "PowerUpUsage.h"

#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/component/PathToOnGrid.h"
#include "snacman/simulations/snacgame/component/PlayerPowerUp.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/system/Pathfinding.h"

#include "../typedef.h"

#include <limits>
#include <math/Box.h>
#include <math/Transformations.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpUsage::update()
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

    mPowUpPlayers.each([this, &powerup](
                           EntHandle aHandle,
                           const component::Geometry & aPlayerGeo,
                           component::PlayerPowerUp & aPowerUp,
                           const component::Controller & aController) {
        if (aController.mCommandQuery == gPlayerUsePowerup
            && mPlayers.countMatches() > 1)
        {
            // Get placement tile
            auto [powerupPos, targetHandle] =
                getPowerupPlacementTile(aHandle, aPlayerGeo);
            // Transfer powerup to level node in scene graph
            // Adds components behavior
            //  TODO: (franz) Animate the player
            transferEntity(aPowerUp.mPowerUp, *mGameContext->mLevel);
            Entity powerupEnt = *aPowerUp.mPowerUp.get(powerup);
            component::Geometry & puGeo = powerupEnt.get<component::Geometry>();
            powerupEnt.add(component::LevelEntity{});
            powerupEnt.add(
                component::AllowedMovement{});
            powerupEnt.add(
                component::PathToOnGrid{.mEntityTarget = targetHandle});

            puGeo.mPosition.x() = powerupPos.x();
            puGeo.mPosition.y() = powerupPos.y();
            puGeo.mPosition.z() = gPillHeight;

            aHandle.get(powerup)->remove<component::PlayerPowerUp>();
        }
    });
}

std::pair<Pos2, EntHandle>
PowerUpUsage::getPowerupPlacementTile(EntHandle aHandle,
                                      const component::Geometry & aGeo)
{
    Phase placement;

    const component::LevelData & lvlData =
        mGameContext->mLevel->get(placement)->get<component::LevelData>();
    const std::vector<component::PathfindNode> & nodes = lvlData.mNodes;
    const size_t stride = lvlData.mSize.width();

    unsigned int currentDepth = std::numeric_limits<unsigned int>::max();
    Pos2 targetPos;
    EntHandle targetHandle = aHandle;

    mPlayers.each([&stride, &aGeo, aHandle, &currentDepth,
                   &targetPos, &targetHandle, nodes](
                      EntHandle aOther, const component::Geometry & aOtherGeo) {
        if (aHandle != aOther)
        {
            // We need a copy of nodes to make the calculation in place
            // so nodes is captured by value
            std::vector<component::PathfindNode> localNodes = nodes;
            component::PathfindNode * targetNode = pathfind(
                aGeo.mPosition.xy(), aOtherGeo.mPosition.xy(), localNodes, stride);

            int newDepth = 0;

            while (targetNode->mPrev != nullptr
                   && targetNode->mPrev->mPrev != nullptr
                   && targetNode->mPrev->mPrev->mPrev != nullptr)
            {
                newDepth++;
                targetNode = targetNode->mPrev;
            }

            if (newDepth < currentDepth)
            {
                targetPos = targetNode->mPos;
                targetHandle = aOther;
            }
        }
    });

    return std::make_pair(targetPos, targetHandle);
}
} // namespace system
} // namespace snacgame
} // namespace ad
