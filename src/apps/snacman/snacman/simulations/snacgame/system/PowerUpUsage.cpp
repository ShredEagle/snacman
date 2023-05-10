#include "PowerUpUsage.h"

#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/SceneNode.h"
#include "snacman/simulations/snacgame/GameParameters.h"

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerPowerUp.h"
#include "../component/PowerUp.h"
#include "../Entities.h"
#include "../InputConstants.h"
#include "../SceneGraph.h"
#include "../system/Pathfinding.h"
#include "../typedef.h"

#include <limits>
#include <math/Box.h>
#include <math/Transformations.h>
#include <optional>
#include <snacman/DebugDrawing.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpUsage::update(float aDelta)
{
    mPowerups.each([&aDelta, this](component::PowerUp & aPowerUp,
                                   component::VisualModel & aVisualModel,
                                   component::Geometry & aGeo) {
        if (aPowerUp.mSwapTimer > 0.f)
        {
            aPowerUp.mSwapTimer -= aDelta;
        }
        else
        {
            aPowerUp.mSwapTimer = 1.f;
            if (aPowerUp.mType == component::PowerUpType::Dog)
            {
                component::PowerUpType newType = component::PowerUpType::Teleport;
                component::PowerUpBaseInfo info =
                    component::gPowerupPathByType.at(static_cast<unsigned int>(
                        newType));
                aVisualModel.mModel =
                    mGameContext->mResources.getModel(info.mPath);
                aPowerUp.mType = newType;
                aGeo.mInstanceScaling = info.mInstanceScale;
                aGeo.mOrientation = component::gBasePowerupQuat * info.mOrientation;
                aGeo.mScaling = info.mScaling;
            }
            else if (aPowerUp.mType == component::PowerUpType::Teleport)
            {
                component::PowerUpType newType = component::PowerUpType::Missile;
                component::PowerUpBaseInfo info =
                    component::gPowerupPathByType.at(static_cast<unsigned int>(
                        newType));
                aVisualModel.mModel =
                    mGameContext->mResources.getModel(info.mPath);
                aPowerUp.mType = newType;
                aGeo.mInstanceScaling = info.mInstanceScale;
                aGeo.mOrientation = component::gBasePowerupQuat * info.mOrientation;
                aGeo.mScaling = info.mScaling;
            }
            else if (aPowerUp.mType == component::PowerUpType::Missile)
            {
                component::PowerUpType newType = component::PowerUpType::Dog;
                component::PowerUpBaseInfo info =
                    component::gPowerupPathByType.at(static_cast<unsigned int>(
                        newType));
                aVisualModel.mModel =
                    mGameContext->mResources.getModel(info.mPath);
                aPowerUp.mType = newType;
                aGeo.mInstanceScaling = info.mInstanceScale;
                aGeo.mOrientation = component::gBasePowerupQuat * info.mOrientation;
                aGeo.mScaling = info.mScaling;
            }
        }
    });
    Phase powerup;
    mPlayers.each([&](EntHandle aPlayer,
                      const component::GlobalPose & aPlayerPose,
                      component::PlayerHud & aHud,
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

                DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug)
                    .addBox(
                        snac::DebugDrawer::Entry{
                            .mPosition = {0.f, 0.f, 0.f},
                            .mColor = math::hdr::gBlue<float>,
                        },
                        powerupHitbox);

                if (!aPowerup.mPickedUp
                    && component::collideWithSat(powerupHitbox, playerHitbox))
                {
                    EntHandle playerPowerup =
                        createPlayerPowerUp(*mGameContext, aPowerup.mType);
                    component::PlayerPowerUp newPowerup = {
                        .mPowerUp = playerPowerup, .mType = aPowerup.mType};
                    aHud.mPowerUpName = component::gPowerUpName.at(
                        static_cast<std::size_t>(aPowerup.mType));
                    insertEntityInScene(playerPowerup,
                                        aPlayer.get(powerup)
                                            ->get<component::PlayerModel>()
                                            .mModel);
                    aPowerup.mPickedUp = true;
                    aHandle.get(powerup)->erase();

                    switch (aPowerup.mType)
                    {
                    case component::PowerUpType::Dog:
                    {
                        break;
                    }
                    case component::PowerUpType::Teleport:
                    {
                        newPowerup.mInfo = component::TeleportPowerUpInfo{};
                        break;
                    }
                    case component::PowerUpType::_End:
                    default:
                        break;
                    }

                    playerEnt.add(newPowerup);
                }
            });
        }
    });

    mPowUpPlayers.each([this, &powerup, &aDelta](
                           EntHandle aHandle,
                           const component::Geometry & aPlayerGeo,
                           component::PlayerPowerUp & aPowerUp,
                           component::GlobalPose & aPlayerPose,
                           const component::Controller & aController) {
        switch (aPowerUp.mType)
        {
        case component::PowerUpType::Dog:
        {
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
                component::Geometry & puGeo =
                    powerupEnt.get<component::Geometry>();
                powerupEnt.add(component::LevelEntity{})
                    .add(component::AllowedMovement{})
                    .add(component::PathToOnGrid{.mEntityTarget = targetHandle})
                    .add(component::Collision{.mHitbox =
                                                  component::gPowerUpHitbox})
                    .add(component::InGamePowerup{.mOwner = aHandle,
                                                  .mType = aPowerUp.mType});

                puGeo.mPosition.x() = powerupPos.x();
                puGeo.mPosition.y() = powerupPos.y();
                puGeo.mPosition.z() = gPillHeight;

                aHandle.get(powerup)->remove<component::PlayerPowerUp>();
            }
            break;
        }
        case component::PowerUpType::Teleport:
        {
            component::TeleportPowerUpInfo & info =
                std::get<component::TeleportPowerUpInfo>(aPowerUp.mInfo);
            if (!info.mCurrentTarget)
            {
                std::optional<EntHandle> firstTarget =
                    getClosestPlayer(aHandle, aPlayerPose.mPosition);
                if (firstTarget)
                {
                    EntHandle arrowHandle =
                        createTargetArrow(*mGameContext, aPlayerGeo.mColor);
                    insertEntityInScene(arrowHandle, *firstTarget);

                    info.mCurrentTarget = firstTarget;
                    info.mTargetArrow = arrowHandle;
                }
            }

            if (aController.mCommandQuery & gPlayerUsePowerup
                && info.mCurrentTarget && info.mCurrentTarget->isValid())
            {
                Phase swapPlayer;
                swapPlayerPosition(swapPlayer, aHandle, *info.mCurrentTarget);
                info.mTargetArrow->get(swapPlayer)->erase();
                aPowerUp.mPowerUp.get(swapPlayer)->erase();
                aHandle.get(swapPlayer)->remove<component::PlayerPowerUp>();
                break;
            }

            bool changeTarget = ((aController.mCommandQuery
                                  & (gNextPowerUpTarget | gPrevPowerUpTarget)))
                                || (aController.mCommandQuery
                                    & (gRightJoyUp | gRightJoyDown
                                       | gRightJoyLeft | gRightJoyRight));

            if (changeTarget)
            {
                // Change
                if (info.mDelayChangeTarget <= 0.f)
                {
                    // This filter the mCommandQuery for only the powerup target
                    // commands and make them into 1 and 2 this then index the
                    // gDirections to get the horizontal plus and minus
                    // directions
                    const Vec2_i direction = gDirections
                        [(aController.mCommandQuery >> gBaseTargetShift & 0b11)
                         + 1];

                    constexpr int otherSlotSize = gMaxPlayerSlots - 1;
                    std::array<std::pair<Pos2, OptEntHandle>, otherSlotSize>
                        mSortedPosition;
                    int positionSize = 0;
                    mPlayers.each([&positionSize, &direction, &mSortedPosition,
                                   &aHandle](
                                      const EntHandle aOther,
                                      const component::GlobalPose & aPose) {
                        if (aOther != aHandle)
                        {
                            // This makes it so we can sort by the direction
                            // pressed by the player
                            mSortedPosition.at(positionSize).first =
                                aPose.mPosition.xy() * direction.x();
                            mSortedPosition.at(positionSize).second = aOther;
                            positionSize += 1;
                        }
                    });
                    // Sort with respect to the direction pressed by the player
                    std::sort(
                        mSortedPosition.begin(), mSortedPosition.end(),
                        [](const std::pair<Pos2, std::optional<EntHandle>> &
                               aLhs,
                           const std::pair<Pos2, std::optional<EntHandle>> &
                               aRhs) -> bool {
                            return !aRhs.second
                                   || (aLhs.second
                                       && (aLhs.first.x() > aRhs.first.x()
                                           || (aLhs.first.x() == aRhs.first.x()
                                               && aRhs.first.y()
                                                      > aLhs.first.y())));
                        });

                    // Find the current target in the array
                    int currentTargetIndex = (int) std::distance(
                        mSortedPosition.begin(),
                        std::find_if(
                            mSortedPosition.begin(), mSortedPosition.end(),
                            [&info](const std::pair<
                                    Pos2, std::optional<EntHandle>> & aItem)
                                -> bool {
                                return aItem.second && info.mCurrentTarget
                                       && *(aItem.second)
                                              == *(info.mCurrentTarget);
                            }));

                    // And chose the next player in the direction the player
                    // chose
                    if (currentTargetIndex < otherSlotSize)
                    {
                        info.mCurrentTarget =
                            *mSortedPosition[(currentTargetIndex + 1)
                                             % positionSize]
                                 .second;
                    }

                    // We then reset the delay
                    info.mDelayChangeTarget =
                        component::gTeleportChangeTargetDelay;
                }
                else
                {
                    info.mDelayChangeTarget -= aDelta;
                }

                Phase updateArrowParent;
                if (info.mCurrentTarget
                    != info.mTargetArrow->get(updateArrowParent)
                           ->get<component::SceneNode>()
                           .mParent)
                {
                    removeEntityFromScene(*info.mTargetArrow);
                    insertEntityInScene(*info.mTargetArrow,
                                        *info.mCurrentTarget);
                }
            }
            else
            {
                info.mDelayChangeTarget = 0.f;
            }
            break;
        }
        case component::PowerUpType::_End:
        default:
            break;
        }
    });

    mInGamePowerups.each([this, &powerup](
                             EntHandle aPowerupHandle,
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
                    aPlayerLifeCycle.mInvulFrameCounter =
                        component::gBaseHitStunDuration;
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

std::optional<EntHandle> PowerUpUsage::getClosestPlayer(EntHandle aPlayer,
                                                        const Pos3 & aPos)
{
    std::optional<EntHandle> result;
    float minDistSquare = std::numeric_limits<float>::max();
    mPlayers.each([&aPlayer, &result, &minDistSquare, &aPos](
                      EntHandle aHandle, const component::GlobalPose & aPose) {
        if (aPlayer != aHandle)
        {
            float dist = (aPose.mPosition - aPos).getNormSquared();
            if (dist < minDistSquare)
            {
                minDistSquare = dist;
                result = aHandle;
            }
        }
    });

    return result;
}
} // namespace system
} // namespace snacgame
} // namespace ad
