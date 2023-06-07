#include "PowerUpUsage.h"

#include "../component/AllowedMovement.h"
#include "../component/Collision.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerSlot.h"
#include "../component/PowerUp.h"
#include "../component/SceneNode.h"
#include "../component/Speed.h"
#include "../component/Text.h"
#include "../component/VisualModel.h"
#include "../component/Tags.h"
#include "../component/LevelData.h"

#include "../system/Pathfinding.h"

#include "../GameContext.h"
#include "../GameParameters.h"
#include "../Entities.h"
#include "../InputConstants.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <snacman/Timing.h>
#include <snacman/DebugDrawing.h>
#include <snacman/EntityUtilities.h>
#include <snacman/Profiling.h>

#include <math/Box.h>
#include <math/VectorUtilities.h>
#include <math/Transformations.h>

#include <limits>
#include <optional>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {

PowerUpUsage::PowerUpUsage(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayers{mGameContext->mWorld},
    mPowUpPlayers{mGameContext->mWorld},
    mPowerups{mGameContext->mWorld},
    mInGameDogPowerups(mGameContext->mWorld),
    mInGameMissilePowerups(mGameContext->mWorld)
{}

void PowerUpUsage::update(const snac::Time & aTime)
{
    TIME_RECURRING_CLASSFUNC(Main);
    const float delta = (float) aTime.mDeltaSeconds;
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

    {
        Phase pickup;
        // Powerup pickup phase
        mPlayers.each([this, &pickup](EntHandle aPlayer,
                          const component::GlobalPose & aPlayerPose,
                          component::Collision aPlayerCol,
                          component::PlayerRoundData & aRoundData) {
            const Box_f playerHitbox = component::transformHitbox(
                aPlayerPose.mPosition, aPlayerCol.mHitbox);
            if (aRoundData.mType == component::PowerUpType::None
                && aRoundData.mInvulFrameCounter <= 0)
            {
                mPowerups.each([&playerHitbox, this, &aPlayer, &pickup, &aRoundData](ent::Handle<ent::Entity> aHandle,
                                   component::PowerUp & aPowerup,
                                   const component::GlobalPose & aPowerupGeo,
                                   const component::Collision & aPowerupCol) {
                    const Box_f powerupHitbox = component::transformHitbox(
                        aPowerupGeo.mPosition, aPowerupCol.mHitbox);

                    DBGDRAW(snac::gHitboxDrawer,
                            snac::DebugDrawer::Level::debug)
                        .addBox(
                            snac::DebugDrawer::Entry{
                                .mPosition = {0.f, 0.f, 0.f},
                                .mColor = math::hdr::gBlue<float>,
                            },
                            powerupHitbox);

                    if (!aPowerup.mPickedUp
                        && component::collideWithSat(powerupHitbox,
                                                     playerHitbox))
                    {
                        EntHandle playerPowerup =
                            createPlayerPowerUp(*mGameContext, aPowerup.mType);
                        aRoundData.mType = aPowerup.mType;
                        aRoundData.mPowerUp = playerPowerup;

                        insertEntityInScene(playerPowerup,
                                            aPlayer.get()
                                                ->get<component::PlayerRoundData>()
                                                .mModel);
                        aPowerup.mPickedUp = true;
                        aHandle.get(pickup)->erase();

                        switch (aRoundData.mType)
                        {
                        case component::PowerUpType::Dog:
                        {
                            break;
                        }
                        case component::PowerUpType::Teleport:
                        {
                            aRoundData.mInfo = component::TeleportPowerUpInfo{};
                            break;
                        }
                        case component::PowerUpType::Missile:
                        {
                            aRoundData.mInfo = component::MissilePowerUpInfo{};
                            break;
                        }
                        default:
                            break;
                        }
                    }
                });
            }
        });
    }

    Phase usage;
    // Power up usage phase
    mPowUpPlayers.each([this, &usage, &delta](
                           EntHandle aHandle,
                           const component::Geometry & aPlayerGeo,
                           component::PlayerRoundData & aRoundData,
                           component::GlobalPose & aPlayerPose,
                           const component::Controller & aController) {
        if (aHandle.get()->has<component::ControllingMissile>())
        {
            return;
        }

        switch (aRoundData.mType)
        {
        case component::PowerUpType::Dog:
        {
            if (aController.mInput.mCommand & gPlayerUsePowerup
                && mPlayers.countMatches() > 1)
            {
                // Get placement tile
                auto [powerupPos, targetHandle] =
                    getDogPlacementTile(aHandle, aPlayerGeo);
                // Transfer powerup to level node in scene graph
                // Adds components behavior
                // TODO: (franz) Animate the player
                // FIX: (franz)Recreate the entity instead of transfering it
                transferEntity(*aRoundData.mPowerUp, *mGameContext->mLevel);
                Entity powerupEnt = *aRoundData.mPowerUp->get(usage);
                component::Geometry & puGeo =
                    powerupEnt.get<component::Geometry>();
                powerupEnt
                    .add(component::AllowedMovement{})
                    .add(component::PathToOnGrid{.mEntityTarget =
                                                     targetHandle})
                    .add(component::Collision{
                        .mHitbox = component::gPowerUpHitbox})
                    .add(component::InGamePowerup{
                        .mOwner = aHandle,
                        .mType = aRoundData.mType,
                        .mInfo = component::InGameDog{}});

                puGeo.mPosition.x() = powerupPos.x();
                puGeo.mPosition.y() = powerupPos.y();
                puGeo.mPosition.z() = 0.f;

                aRoundData.mType = component::PowerUpType::None;
            }
            break;
        }
        case component::PowerUpType::Teleport:
        {
            component::PlayerSlot & playerSlot = snac::getComponent<component::PlayerSlot>(aRoundData.mSlotHandle);
            component::TeleportPowerUpInfo & info =
                std::get<component::TeleportPowerUpInfo>(aRoundData.mInfo);
            if (!info.mCurrentTarget)
            {
                OptEntHandle firstTarget =
                    getClosestPlayer(aHandle, aPlayerPose.mPosition);
                if (firstTarget)
                {
                    EntHandle arrowHandle = createTargetArrow(
                        *mGameContext, gSlotColors.at(playerSlot.mSlotIndex));
                    insertEntityInScene(arrowHandle, *firstTarget);

                    info.mCurrentTarget = firstTarget;
                    info.mTargetArrow = arrowHandle;
                }
            }

            // This uses the powerup so this ends the switch statement
            if (aController.mInput.mCommand & gPlayerUsePowerup
                && info.mCurrentTarget && info.mCurrentTarget->isValid())
            {
                swapPlayerPosition(usage, aHandle, *info.mCurrentTarget);
                info.mTargetArrow->get(usage)->erase();
                aRoundData.mPowerUp->get(usage)->erase();
                aHandle.get(usage)->remove<component::PlayerPowerUp>();

                auto removePortalImage = [&usage, &aRoundData](EntHandle aInPortalHandle) {
                    if (aRoundData.mPortalImage)
                    {
                        aRoundData.mCurrentPortal = -1;
                        aRoundData.mDestinationPortal = -1;

                        aRoundData.mPortalImage->get(usage)->erase();
                        aRoundData.mPortalImage = std::nullopt;
                    }
                };
                removePortalImage(*info.mCurrentTarget);
                removePortalImage(aHandle);
                break;
            }

            bool changeTarget =
                ((aController.mInput.mCommand
                  & (gNextPowerUpTarget | gPrevPowerUpTarget)))
                || (aController.mInput.mCommand
                    & (gRightJoyUp | gRightJoyDown | gRightJoyLeft
                       | gRightJoyRight));

            if (changeTarget)
            {
                // Change
                if (info.mDelayChangeTarget <= 0.f)
                {
                    // This filter the mCommandQuery for only the powerup
                    // target commands and make them into 1 and 2 this then
                    // index the gDirections to get the horizontal plus and
                    // minus directions
                    const Vec2_i direction =
                        gDirections[(aController.mInput.mCommand
                                         >> gBaseTargetShift
                                     & 0b11)
                                    + 1];

                    constexpr int otherSlotSize = gMaxPlayerSlots - 1;
                    std::array<std::pair<Pos2, OptEntHandle>, otherSlotSize>
                        mSortedPosition;
                    int positionSize = 0;
                    mPlayers.each(
                        [&positionSize, &direction, &mSortedPosition,
                         &aHandle](const EntHandle aOther,
                                   const component::GlobalPose & aPose) {
                            if (aOther != aHandle)
                            {
                                // This makes it so we can sort by the
                                // direction pressed by the player
                                mSortedPosition.at(positionSize).first =
                                    aPose.mPosition.xy() * direction.x();
                                mSortedPosition.at(positionSize).second =
                                    aOther;
                                positionSize += 1;
                            }
                        });
                    // Sort with respect to the direction pressed by the
                    // player
                    std::sort(
                        mSortedPosition.begin(), mSortedPosition.end(),
                        [](const std::pair<Pos2, OptEntHandle> & aLhs,
                           const std::pair<Pos2, OptEntHandle> & aRhs)
                            -> bool {
                            // In case the two are considered "equal", we
                            // must return false
                            if (!aRhs.second && !aLhs.second)
                            {
                                return false;
                            }
                            else
                            {
                                return !aRhs.second
                                       || (aLhs.second
                                           && (aLhs.first.x()
                                                   > aRhs.first.x()
                                               || (aLhs.first.x()
                                                       == aRhs.first.x()
                                                   && aRhs.first.y()
                                                          > aLhs.first
                                                                .y())));
                            }
                        });

                    // Find the current target in the array
                    int currentTargetIndex = (int) std::distance(
                        mSortedPosition.begin(),
                        std::find_if(
                            mSortedPosition.begin(), mSortedPosition.end(),
                            [&info](
                                const std::pair<Pos2, OptEntHandle> & aItem)
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

                if (info.mCurrentTarget
                    != info.mTargetArrow->get(usage)
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
                component::PlayerSlot & playerSlot = snac::getComponent<component::PlayerSlot>(aRoundData.mSlotHandle);
                EntHandle rootPowerup = *aRoundData.mPowerUp;
                Entity rootPowerupEnt = *rootPowerup.get(usage);
                component::Geometry & puGeo =
                    rootPowerupEnt.get<component::Geometry>();
                rootPowerupEnt.remove<component::VisualModel>();
                component::Geometry & playerModelGeo =
                    aRoundData.mModel.get()->get<component::Geometry>();
                Quat_f missileOrientation = puGeo.mOrientation;

                // Transfer powerup to level node in scene graph
                transferEntity(rootPowerup, *mGameContext->mLevel);
                puGeo.mOrientation = Quat_f::Identity();
                puGeo.mScaling = 1.f;
                puGeo.mInstanceScaling = {1.f, 1.f, 1.f};
                puGeo.mPosition.z() = 0.f;

                EntHandle ring = mGameContext->mWorld.addEntity();
                {
                    Phase ringPhase;
                    Entity ringEnt = *ring.get(ringPhase);
                    // TODO: (franz) put in Entities.cpp the creation of the
                    // area
                    addMeshGeoNode(*mGameContext, ringEnt,
                                   "models/missile/area.gltf",
                                   "effects/MeshTextures.sefx",
                                   {0.f, 0.f, 0.f}, 3.f, {1.f, 1.f, 1.f},
                                   Quat_f{UnitVec3{Vec3{1.f, 0.f, 0.f}},
                                          Turn_f{0.25f}},
                                   gSlotColors.at(playerSlot.mSlotIndex));
                    ringEnt.add(component::RoundTransient{});
                }

                EntHandle missileModel = mGameContext->mWorld.addEntity();
                {
                    Phase missilePhase;
                    constexpr component::PowerUpBaseInfo info =
                        component::gPowerupInfoByType[(
                            unsigned int) component::PowerUpType::Missile];
                    Entity missileEnt = *missileModel.get(missilePhase);
                    addMeshGeoNode(
                        *mGameContext, missileEnt, info.mPath,
                        info.mProgPath, {0.f, 0.f, gPillHeight},
                        info.mPlayerScaling, info.mPlayerInstanceScale,
                        playerModelGeo.mOrientation * missileOrientation);
                    missileEnt.add(component::RoundTransient{});
                }
                insertEntityInScene(missileModel, rootPowerup);
                insertEntityInScene(ring, rootPowerup);

                // Adds components behavior
                component::InGamePowerup inGamePowerup{
                    .mOwner = aHandle,
                    .mType = aRoundData.mType,
                    .mInfo = component::InGameMissile{
                        .mModel = missileModel,
                        .mDamageArea = ring,
                    }};
                math::Vec<3, float> mForward = missileOrientation.rotate(
                    math::Vec<3, float>{0.f, 1.f, 0.f});

                rootPowerupEnt.add(component::RoundTransient{})
                    .add(component::Speed{.mSpeed = mForward})
                    .add(inGamePowerup);

                aHandle.get(usage)->remove<component::PlayerPowerUp>();
                aHandle.get(usage)->add(component::ControllingMissile{});
            }
            break;
        }
        default:
            break;
        }
    });

    {
        Phase inGameDog;
        mInGameDogPowerups.each([this, &inGameDog, &aTime](
                                    EntHandle aPowerupHandle,
                                    const component::GlobalPose & aPowerupPose,
                                    const component::Geometry & aGeo,
                                    const component::Collision & aPowerupCol,
                                    component::InGamePowerup & aPowerup) {
            const Box_f powerupHitbox = component::transformHitbox(
                aPowerupPose.mPosition, aPowerupCol.mHitbox);
            mPlayers.each([&aPowerup, powerupHitbox, &aPowerupHandle,
                           &inGameDog, &aGeo, this, &aTime](
                              EntHandle aPlayerHandle,
                              const component::GlobalPose & aPlayerPose,
                              const component::Collision & aPlayerCol,
                              component::PlayerRoundData & aPlayerRoundData) {
                if (aPlayerHandle != aPowerup.mOwner)
                {
                    const Box_f playerHitbox = component::transformHitbox(
                        aPlayerPose.mPosition, aPlayerCol.mHitbox);

                    if (component::collideWithSat(powerupHitbox, playerHitbox))
                    {
                        // TODO: (franz): make hitstun dependent on
                        // powerup type
                        aPlayerRoundData.mInvulFrameCounter =
                            component::gBaseHitStunDuration;
                        aPowerupHandle.get(inGameDog)->erase();
                        EntHandle explosionHandle = createExplosion(
                            *mGameContext, aGeo.mPosition, aTime);
                        insertEntityInScene(explosionHandle,
                                            *mGameContext->mLevel);
                    }
                }
            });
        });
    }

    {
        Phase manageMissile;
        mInGameMissilePowerups.each([&manageMissile, delta, this, &aTime](
                                        EntHandle aPowerupHandle,
                                        const component::GlobalPose &
                                            aPowerupPose,
                                        component::Geometry & aGeo,
                                        component::Speed & aSpeed,
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
            component::InGameMissile info =
                std::get<component::InGameMissile>(aPowerup.mInfo);
            component::Geometry & modelGeo =
                info.mModel.get(manageMissile)->get<component::Geometry>();
            component::GlobalPose & modelGlobalPose =
                info.mModel.get(manageMissile)->get<component::GlobalPose>();

            // Make time tick
            info.mDelayExplosion -= delta;

            if (dirInputPresent)
            {
                // We make the missile up right with the first term and then we
                // compute The proper orientation according to the directional
                // input
                math::Angle<float> dirAngle{
                    (float) getOrientedAngle(rightDir, Vec2{0.f, 1.f}).value()};
                modelGeo.mOrientation =
                    Quat_f{UnitVec3{Vec3{0.f, 0.f, 1.f}}, dirAngle};
            }

            DBGDRAW_DEBUG(snac::gHitboxDrawer)
                .addBasis({.mPosition = modelGlobalPose.mPosition,
                           .mOrientation = modelGlobalPose.mOrientation});
            DBGDRAW_DEBUG(snac::gHitboxDrawer)
                .addBasis({.mPosition =
                               modelGlobalPose.mPosition + Vec3{1.f, 0.f, 0.f},
                           .mOrientation = aPowerupPose.mOrientation});

            // Compute speed from orientation
            math::Vec<3, float> mForward =
                (modelGeo.mOrientation)
                    .rotate(math::Vec<3, float>{0.f, 1.f, 0.f});
            aSpeed.mSpeed = mForward * component::gMissileSpeed;

            const bool explodeMissile =
                command & gPlayerUsePowerup || info.mDelayExplosion <= 0.f;

            if (explodeMissile)
            {
                // Boom boom the missile
                // Find all player within radius
                mPlayers.each([&aPowerupPose](
                                  EntHandle aOther,
                                  component::PlayerRoundData & aPlayerRoundData,
                                  component::GlobalPose & aPlayerPose) {
                    float distance =
                        (aPowerupPose.mPosition - aPlayerPose.mPosition)
                            .getNormSquared();

                    if (distance < 2.f * 2.f)
                    {
                        aPlayerRoundData.mInvulFrameCounter =
                            component::gBaseHitStunDuration;
                    }
                });

                // remove missile entity
                component::InGameMissile & missileInfo =
                    std::get<component::InGameMissile>(aPowerup.mInfo);
                missileInfo.mDamageArea.get(manageMissile)->erase();
                missileInfo.mModel.get(manageMissile)->erase();
                aPowerupHandle.get(manageMissile)->erase();
                aOwner.get(manageMissile)
                    ->remove<component::ControllingMissile>();

                EntHandle explosionHandle =
                    createExplosion(*mGameContext, aGeo.mPosition, aTime);
                insertEntityInScene(explosionHandle, *mGameContext->mLevel);
            }
        });
    }

    // Update power-up name in HUD
    mPlayers.each([](ent::Handle<ent::Entity> aPlayer,
                     component::PlayerRoundData & aRoundData) {
        // TODO code smell, this is defensive programming because sometimes we
        // get there when the round monitor already removed the hud from the
        // entitymanager (I suppose the correct logic would be not to execute
        // this system on players between rounds)
        
        OptEntHandle hud = snac::getComponent<component::PlayerGameData>(aRoundData.mSlotHandle).mHud;
        if (hud && hud->isValid() && aRoundData.mType != component::PowerUpType::None)
        {
            auto & playerHud =
                snac::getComponent<component::PlayerHud>(*hud);
            snac::getComponent<component::Text>(playerHud.mPowerupText)
                .mString = component::getPowerUpName(aPlayer);
        }
    });
}

std::pair<Pos2, EntHandle>
PowerUpUsage::getDogPlacementTile(EntHandle aHandle,
                                  const component::Geometry & aGeo)
{
    const component::Level & lvlData =
        mGameContext->mLevel->get()->get<component::Level>();
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
                component::PathfindNode targetNode =
                    pathfind(aGeo.mPosition.xy(), aOtherGeo.mPosition.xy(),
                             localNodes, stride);

                unsigned int newDepth = 0;

                while (targetNode.mPrev != nullptr
                       && targetNode.mPrev->mPrev != nullptr
                       && targetNode.mPrev->mPrev->mPrev != nullptr)
                {
                    newDepth++;
                    targetNode = *targetNode.mPrev;
                }

                if (newDepth < currentDepth)
                {
                    targetPos = targetNode.mPos;
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
