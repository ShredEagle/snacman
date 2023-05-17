#include "PowerUpUsage.h"

#include "math/VectorUtilities.h"
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/SceneNode.h"
#include "snacman/simulations/snacgame/component/Speed.h"
#include "snacman/simulations/snacgame/component/VisualModel.h"
#include "snacman/simulations/snacgame/GameParameters.h"
#include <snacman/EntityUtilities.h>

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerLifeCycle.h"
#include "../component/PlayerPowerUp.h"
#include "../component/PowerUp.h"
#include "../component/Text.h"

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
#include <snacman/Profiling.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {
void PowerUpUsage::update(const snac::Time & aTime)
{
    TIME_RECURRING_CLASSFUNC(Main);
    const float delta = (float)aTime.mDeltaSeconds;
    mPowerups.each([&delta, this](component::PowerUp & aPowerUp,
                                   component::VisualModel & aVisualModel,
                                   component::Geometry & aGeo) {
        if (aPowerUp.mSwapTimer > 0.f)
        {
            aPowerUp.mSwapTimer -= delta;
        }
        else
        {
            aPowerUp.mSwapTimer = aPowerUp.mSwapPeriod;
            // Loops powerup
            component::PowerUpType newType =
                static_cast<component::PowerUpType>(
                    (static_cast<unsigned int>(aPowerUp.mType) + 1)
                    % static_cast<unsigned int>(component::PowerUpType::_End));

            component::PowerUpBaseInfo info = component::gPowerupInfoByType.at(
                static_cast<unsigned int>(newType));
            // TODO: (franz) put the program in the powerup info
            aVisualModel.mModel = mGameContext->mResources.getModel(
                info.mPath, "effects/MeshTextures.sefx");
            aPowerUp.mType = newType;
            aGeo.mInstanceScaling = info.mLevelInstanceScale;
            aGeo.mOrientation =
                component::gLevelBasePowerupQuat * info.mLevelOrientation;
            aGeo.mScaling = info.mLevelScaling;
        }
    });

    Phase pickup;
    // Powerup pickup phase
    mPlayers.each([&](EntHandle aPlayer,
                      const component::GlobalPose & aPlayerPose,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol) {
        const Box_f playerHitbox = component::transformHitbox(
            aPlayerPose.mPosition, aPlayerCol.mHitbox);
        Entity playerEnt = *aPlayer.get(pickup);
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
                    
                    insertEntityInScene(playerPowerup,
                                        aPlayer.get(pickup)
                                            ->get<component::PlayerModel>()
                                            .mModel);
                    aPowerup.mPickedUp = true;
                    aHandle.get(pickup)->erase();

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
                    case component::PowerUpType::Missile:
                    {
                        newPowerup.mInfo = component::MissilePowerUpInfo{};
                        break;
                    }
                    case component::PowerUpType::_End:
                        break;
                    }

                    playerEnt.add(newPowerup);
                }
            });
        }
    });

    Phase usage;
    // Power up usage phase
    mPowUpPlayers.each([this, &usage, &delta](
                           EntHandle aHandle,
                           const component::Geometry & aPlayerGeo,
                           component::PlayerPowerUp & aPowerUp,
                           component::PlayerSlot & aPlayerSlot,
                           component::GlobalPose & aPlayerPose,
                           const component::Controller & aController) {
        switch (aPowerUp.mType)
        {
        case component::PowerUpType::Dog:
        {
            if (aController.mInput.mCommand & gPlayerUsePowerup)
            {
                // Get placement tile
                auto [powerupPos, targetHandle] =
                    getDogPlacementTile(aHandle, aPlayerGeo);
                // Transfer powerup to level node in scene graph
                // Adds components behavior
                //  TODO: (franz) Animate the player
                transferEntity(aPowerUp.mPowerUp, *mGameContext->mLevel);
                Entity powerupEnt = *aPowerUp.mPowerUp.get(usage);
                component::Geometry & puGeo =
                    powerupEnt.get<component::Geometry>();
                powerupEnt.add(component::LevelEntity{})
                    .add(component::AllowedMovement{})
                    .add(component::PathToOnGrid{.mEntityTarget = targetHandle})
                    .add(component::Collision{.mHitbox =
                                                  component::gPowerUpHitbox})
                    .add(component::InGamePowerup{
                        .mOwner = aHandle,
                        .mType = aPowerUp.mType,
                        .mInfo = component::InGameDog{}});

                puGeo.mPosition.x() = powerupPos.x();
                puGeo.mPosition.y() = powerupPos.y();
                puGeo.mPosition.z() = gPillHeight;

                aHandle.get(usage)->remove<component::PlayerPowerUp>();
            }
            break;
        }
        case component::PowerUpType::Teleport:
        {
            component::TeleportPowerUpInfo & info =
                std::get<component::TeleportPowerUpInfo>(aPowerUp.mInfo);
            if (!info.mCurrentTarget)
            {
                OptEntHandle firstTarget =
                    getClosestPlayer(aHandle, aPlayerPose.mPosition);
                if (firstTarget)
                {
                    EntHandle arrowHandle =
                        createTargetArrow(*mGameContext, aPlayerSlot.mColor);
                    insertEntityInScene(arrowHandle, *firstTarget);

                    info.mCurrentTarget = firstTarget;
                    info.mTargetArrow = arrowHandle;
                }
            }

            // This uses the powerup so this ends the switch statement
            if (aController.mInput.mCommand & gPlayerUsePowerup
                && info.mCurrentTarget && info.mCurrentTarget->isValid())
            {
                Phase swapPlayer;
                swapPlayerPosition(swapPlayer, aHandle, *info.mCurrentTarget);
                info.mTargetArrow->get(swapPlayer)->erase();
                aPowerUp.mPowerUp.get(swapPlayer)->erase();
                aHandle.get(swapPlayer)->remove<component::PlayerPowerUp>();
                break;
            }

            bool changeTarget = ((aController.mInput.mCommand
                                  & (gNextPowerUpTarget | gPrevPowerUpTarget)))
                                || (aController.mInput.mCommand
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
                    const Vec2_i direction =
                        gDirections[(aController.mInput.mCommand
                                         >> gBaseTargetShift
                                     & 0b11)
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
                        [](const std::pair<Pos2, OptEntHandle> & aLhs,
                           const std::pair<Pos2, OptEntHandle> & aRhs) -> bool {
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
                            [&info](const std::pair<Pos2, OptEntHandle> & aItem)
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
                    info.mDelayChangeTarget -= delta;
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
        case component::PowerUpType::Missile:
        {
            if (aController.mInput.mCommand & gPlayerUsePowerup)
            {
                // TODO: (franz) Animate the player
                // TODO: (franz) redo this completely to have the model as a
                // child from the creation of powerup
                EntHandle rootPowerup = aPowerUp.mPowerUp;
                Entity rootPowerupEnt = *rootPowerup.get(usage);
                component::Geometry & puGeo =
                    rootPowerupEnt.get<component::Geometry>();
                rootPowerupEnt.remove<component::VisualModel>();
                Quat_f missileOrientation = puGeo.mOrientation;

                // Transfer powerup to level node in scene graph
                transferEntity(rootPowerup, *mGameContext->mLevel);
                puGeo.mOrientation = Quat_f::Identity();
                puGeo.mScaling = 1.f;
                puGeo.mInstanceScaling = {1.f, 1.f, 1.f};
                puGeo.mPosition.z() = 0.f;

                EntHandle ring = mGameContext->mWorld.addEntity();
                {
                    Phase createRing;
                    Entity ringEnt = *ring.get(createRing);
                    // TODO: (franz) put in Entities.cpp the creation of the area
                    addMeshGeoNode(
                        *mGameContext, ringEnt, "models/missile/area.gltf",
                        "effects/MeshTextures.sefx", {0.f, 0.f, 0.f}, 3.f,
                        {1.f, 1.f, 1.f},
                        Quat_f{UnitVec3{Vec3{1.f, 0.f, 0.f}}, Turn_f{0.25f}},
                        aPlayerSlot.mColor);
                    ringEnt.add(component::LevelEntity{});
                }

                EntHandle missileModel = mGameContext->mWorld.addEntity();
                {
                    Phase createModel;
                    constexpr component::PowerUpBaseInfo info =
                        component::gPowerupInfoByType[(
                            unsigned int) component::PowerUpType::Missile];
                    Entity missileEnt = *missileModel.get(createModel);
                    addMeshGeoNode(*mGameContext, missileEnt, info.mPath,
                                   info.mProgPath, {0.f, 0.f, gPillHeight},
                                   info.mPlayerScaling,
                                   info.mPlayerInstanceScale, missileOrientation);
                    missileEnt.add(component::LevelEntity{});
                }
                insertEntityInScene(missileModel, rootPowerup);
                insertEntityInScene(ring, rootPowerup);

                // Adds components behavior
                component::InGamePowerup inGamePowerup{
                    .mOwner = aHandle,
                    .mType = aPowerUp.mType,
                    .mInfo = component::InGameMissile{
                        .mModel = missileModel,
                        .mDamageArea = ring,
                    }};
                math::Vec<3, float> mForward = missileOrientation.rotate(
                    math::Vec<3, float>{0.f, 1.f, 0.f});

                rootPowerupEnt.add(component::LevelEntity{})
                    .add(component::Speed{.mSpeed = mForward})
                    .add(inGamePowerup);

                aHandle.get(usage)->remove<component::PlayerPowerUp>();
            }
            break;
        }
        case component::PowerUpType::_End:
            break;
        }
    });

    Phase inGameDog;
    mInGameDogPowerups.each([this, &inGameDog](
                                EntHandle aPowerupHandle,
                                const component::GlobalPose & aPowerupPose,
                                const component::Geometry & aGeo,
                                const component::Collision & aPowerupCol,
                                component::InGamePowerup & aPowerup) {
        const Box_f powerupHitbox = component::transformHitbox(
            aPowerupPose.mPosition, aPowerupCol.mHitbox);
        mPlayers.each([&aPowerup, powerupHitbox, &aPowerupHandle, &inGameDog](
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
                    // TODO: (franz): make hitstun dependent on
                    // powerup type
                    aPlayerLifeCycle.mHitStun = component::gBaseHitStunDuration;
                    aPlayerLifeCycle.mInvulFrameCounter =
                        component::gBaseHitStunDuration;
                    aPowerupHandle.get(inGameDog)->erase();
                }
            }
        });
    });

    Phase manageMissile;
    mInGameMissilePowerups.each(
        [&manageMissile, delta, this, &aTime](EntHandle aPowerupHandle,
                         const component::GlobalPose & aPowerupPose,
                         component::Geometry & aGeo, component::Speed & aSpeed,
                         component::InGamePowerup & aPowerup) {
            // Change orientation
            EntHandle aOwner = aPowerup.mOwner;
            const component::Controller & aController =
                aOwner.get(manageMissile)->get<component::Controller>();
            const int command = aController.mInput.mCommand;
            const Vec2 rightDir = aController.mInput.mRightDirection;
            const bool dirInputPresent = command
                                         & (gRightJoyUp | gRightJoyDown
                                            | gRightJoyLeft | gRightJoyRight);
            component::InGameMissile info = std::get<component::InGameMissile>(aPowerup.mInfo);
            component::Geometry & modelGeo =
                info.mModel.get(manageMissile)
                    ->get<component::Geometry>();
            component::GlobalPose & modelGlobalPose =
                info.mModel.get(manageMissile)
                    ->get<component::GlobalPose>();

            // Make time tick
            info.mDelayExplosion -= delta;

            if (dirInputPresent)
            {
                // We make the missile up right with the first term and then we
                // compute The proper orientation according to the directional
                // input
                /* modelGeo.mOrientation = Quat_f::Identity(); */
                math::Angle<float> dirAngle{(float)getOrientedAngle(rightDir, Vec2{0.f, 1.f}).value()};
                modelGeo.mOrientation =
                    Quat_f{UnitVec3{Vec3{0.f, 0.f, 1.f}}, dirAngle}; 
            }

            DBGDRAW_DEBUG(snac::gHitboxDrawer).addBasis({.mPosition = modelGlobalPose.mPosition,
                          .mOrientation = modelGlobalPose.mOrientation});
            DBGDRAW_DEBUG(snac::gHitboxDrawer).addBasis({.mPosition = modelGlobalPose.mPosition + Vec3{1.f, 0.f, 0.f},
                          .mOrientation = aPowerupPose.mOrientation});

            // Compute speed from orientation
            math::Vec<3, float> mForward = (modelGeo.mOrientation).rotate(
                math::Vec<3, float>{0.f, 1.f, 0.f});
            aSpeed.mSpeed = mForward * component::gMissileSpeed;

            const bool explodeMissile = command & gPlayerUsePowerup || info.mDelayExplosion <= 0.f;

            if (explodeMissile)
            {
                // Boom boom the missile
                // Find all player within radius
                mPlayers.each([&aPowerupPose, aOwner](EntHandle aOther, component::PlayerLifeCycle & aPlayerLifeCycle, component::GlobalPose & aPlayerPose)
                {
                    float distance = (aPowerupPose.mPosition - aPlayerPose.mPosition).getNormSquared();

                    if (distance < 2.f * 2.f && aOwner != aOther)
                    {
                        aPlayerLifeCycle.mHitStun = component::gBaseHitStunDuration;
                        aPlayerLifeCycle.mInvulFrameCounter = component::gBaseHitStunDuration;
                    }
                });

                // remove missile entity
                component::InGameMissile & missileInfo = std::get<component::InGameMissile>(aPowerup.mInfo);
                missileInfo.mDamageArea.get(manageMissile)->erase();
                missileInfo.mModel.get(manageMissile)->erase();
                aPowerupHandle.get(manageMissile)->erase();

                EntHandle explosionHandle = createExplosion(*mGameContext, aGeo.mPosition, aTime);
                insertEntityInScene(explosionHandle, *mGameContext->mLevel);
            }
        });
                    
    // Update power-up name in HUD
    mPlayers.each([](ent::Handle<ent::Entity> aPlayer, component::PlayerLifeCycle & aLifeCycle)
    {
        // TODO code smell, this is defensive programming because sometimes we get there
        // when the round monitor already removed the hud from the entitymanager
        // (I suppose the correct logic would be not to execute this system on players between rounds)
        if(aLifeCycle.mHud && aLifeCycle.mHud->isValid())
        {
            auto & playerHud = snac::getComponent<component::PlayerHud>(*aLifeCycle.mHud);
            snac::getComponent<component::Text>(playerHud.mPowerupText)
                    .mString = component::getPowerUpName(aPlayer);
        }
    });
}

std::pair<Pos2, EntHandle>
PowerUpUsage::getDogPlacementTile(EntHandle aHandle,
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

OptEntHandle PowerUpUsage::getClosestPlayer(EntHandle aPlayer,
                                            const Pos3 & aPos)
{
    OptEntHandle result;
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
