#include "PowerUpUsage.h"

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerPowerUp.h"
#include "../Entities.h"
#include "../InputConstants.h"
#include "../SceneGraph.h"
#include "../system/Pathfinding.h"

#include "../typedef.h"

#include <snacman/DebugDrawing.h>
#include <math/Box.h>
#include <math/Transformations.h>
#include <utility>
#include <limits>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpUsage::update()
{
    Phase powerup;
    mPlayers.each([&](EntHandle aPlayer, const component::GlobalPose & aPlayerPose,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol) {
        const Box_f playerHitbox = component::transformHitbox(
            aPlayerPose.mPosition, aPlayerCol.mHitbox);
        Entity playerEnt = *aPlayer.get(powerup);
        if (!playerEnt.has<component::PlayerPowerUp>())
        {
            mPowerups.each([&](ent::Handle<ent::Entity> aHandle,
                               component::PowerUp & aPowerup,
                               const component::GlobalPose & aPowerupGeo,
                               const component::Collision & aPowerupCol) {
                const Box_f powerupHitbox = component::transformHitbox(
                    aPowerupGeo.mPosition, aPowerupCol.mHitbox);

                DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::off).addBox(
                    snac::DebugDrawer::Entry{
                        .mPosition = {0.f, 0.f, 0.f},
                        .mColor = math::hdr::gBlue<float>,
                    },
                    powerupHitbox
                );

                if (!aPowerup.mPickedUp
                    && component::collideWithSat(powerupHitbox, playerHitbox))
                {
                    EntHandle aPlayerPowerup =
                        createPlayerPowerUp(*mGameContext);
                    playerEnt.add(
                        component::PlayerPowerUp{.mPowerUp = aPlayerPowerup,
                                                 .mType = aPowerup.mType});
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
        if (aController.mCommandQuery & gPlayerUsePowerup
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
            powerupEnt.add(component::LevelEntity{})
                .add(component::AllowedMovement{})
                .add(component::PathToOnGrid{.mEntityTarget = targetHandle})
                .add(
                    component::Collision{.mHitbox = component::gPowerUpHitbox})
                .add(component::InGamePowerup{.mOwner = aHandle, .mType = aPowerUp.mType});

            puGeo.mPosition.x() = powerupPos.x();
            puGeo.mPosition.y() = powerupPos.y();
            puGeo.mPosition.z() = gPillHeight;

            aHandle.get(powerup)->remove<component::PlayerPowerUp>();
        }
    });

    mInGamePowerups.each([this, &powerup](EntHandle aPowerupHandle,
                                const component::GlobalPose & aPowerupPose,
                                const component::Collision & aPowerupCol,
                                component::InGamePowerup & aPowerup) {
        const Box_f powerupHitbox = component::transformHitbox(
            aPowerupPose.mPosition, aPowerupCol.mHitbox);
        mPlayers.each([&aPowerup, powerupHitbox, &aPowerupHandle, &powerup](
                          EntHandle aPlayerHandle,
                          const component::GlobalPose & aPlayerPose,
                          const component::Collision & aPlayerCol,
                          component::PlayerLifeCycle & aPlayerLifeCycle) {
            if (aPlayerHandle != aPowerup.mOwner)
            {
                const Box_f playerHitbox = component::transformHitbox(
                    aPlayerPose.mPosition, aPlayerCol.mHitbox);

                if (component::collideWithSat(powerupHitbox, playerHitbox))
                {
                    // TODO: (franz): make hitstun dependent on powerup
                    // type
                    aPlayerLifeCycle.mHitStun = component::gBaseHitStunDuration;
                    aPlayerLifeCycle.mInvulFrameCounter = component::gBaseHitStunDuration;
                    aPowerupHandle.get(powerup)->erase();
                }
            }
        });
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

    mPlayers.each(
        [&stride, &aGeo, aHandle, &currentDepth, &targetPos, &targetHandle,
         &nodes](EntHandle aOther, const component::Geometry & aOtherGeo) {
            if (aHandle != aOther)
            {
                // We need a copy of nodes to make the calculation in place
                // so nodes is captured by value
                std::vector<component::PathfindNode> localNodes = nodes;
                component::PathfindNode * targetNode =
                    pathfind(aGeo.mPosition.xy(), aOtherGeo.mPosition.xy(),
                             localNodes, stride);

                unsigned int newDepth = 0;

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
