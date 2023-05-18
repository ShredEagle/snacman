#include "Entities.h"

#include "component/AllowedMovement.h"
#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Explosion.h"
#include "component/Geometry.h"
#include "component/LevelTags.h"
#include "component/MenuItem.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerPowerUp.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/PowerUp.h"
#include "component/RigAnimation.h"
#include "component/SceneNode.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"
#include "component/VisualModel.h"
#include "GameContext.h"
#include "scene/MenuScene.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/PlayerHud.h"
#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/PlayerPortalData.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "typedef.h"

#include "../../QueryManipulation.h"
#include "../../Resources.h"

#include <snacman/EntityUtilities.h>

#include <algorithm>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <map>
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>
#include <optional>
#include <ostream>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ad {
namespace snacgame {

constexpr Size3 lLevelElementScaling = {1.f, 1.f, 1.f};

// TODO phase is not used...
void addGeoNode(GameContext & aContext,
                Entity & aEnt,
                Pos3 aPos,
                float aScale,
                Size3 aInstanceScale,
                Quat_f aOrientation,
                HdrColor_f aColor)
{
    aEnt.add(component::Geometry{.mPosition = aPos,
                                 .mScaling = aScale,
                                 .mInstanceScaling = aInstanceScale,
                                 .mOrientation = aOrientation,
                                 .mColor = aColor})
        .add(component::SceneNode{})
        .add(component::GlobalPose{});
}

std::shared_ptr<snac::Model> addMeshGeoNode(GameContext & aContext,
                                            Entity & aEnt,
                                            const char * aModelPath,
                                            const char * aEffectPath,
                                            Pos3 aPos,
                                            float aScale,
                                            Size3 aInstanceScale,
                                            Quat_f aOrientation,
                                            HdrColor_f aColor)
{
    auto model = aContext.mResources.getModel(aModelPath, aEffectPath);
    aEnt.add(component::Geometry{.mPosition = aPos,
                                 .mScaling = aScale,
                                 .mInstanceScaling = aInstanceScale,
                                 .mOrientation = aOrientation,
                                 .mColor = aColor})
        .add(component::VisualModel{.mModel = model})
        .add(component::SceneNode{})
        .add(component::GlobalPose{});

    return model;
}

namespace {
void createLevelElement(Phase & aPhase,
                        EntHandle aHandle,
                        GameContext & aContext,
                        const math::Position<2, float> & aGridPos,
                        HdrColor_f aColor)
{
    Entity path = *aHandle.get(aPhase);
    addMeshGeoNode(
        aContext, path, "models/square_biscuit/square_biscuit.gltf",
        "effects/MeshTextures.sefx",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gLevelHeight},
        0.45f, lLevelElementScaling,
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}},
        aColor);
    path.add(component::LevelEntity{});
}
} // namespace

ent::Handle<ent::Entity> createWorldText(GameContext & aContext,
                                         std::string aText,
                                         component::GlobalPose aPose)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    Entity text = *handle.get(init);

    auto font =
        aContext.mResources.getFont("fonts/FredokaOne-Regular.ttf", 120);

    text.add(component::Text{
                 .mString{std::move(aText)},
                 .mFont = std::move(font),
                 .mColor = math::hdr::gYellow<float>,
             })
        .add(aPose)
        .add(component::LevelEntity{});

    return handle;
}

ent::Handle<ent::Entity>
createAnimatedTest(GameContext & aContext,
                   Phase & aPhase,
                   snac::Clock::time_point aStartTime,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity entity = *handle.get(aPhase);
    std::shared_ptr<snac::Model> model = addMeshGeoNode(
        aContext, entity, "models/anim/anim.gltf", "effects/MeshRigging.sefx",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gLevelHeight},
        0.45f, lLevelElementScaling,
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}});
    const snac::NodeAnimation & animation = model->mAnimations.begin()->second;
    entity.add(component::RigAnimation{
        .mAnimation = &animation,
        .mStartTime = aStartTime,
        .mParameter =
            decltype(component::RigAnimation::mParameter){animation.mEndTime},
    });
    entity.add(component::LevelEntity{});

    return handle;
}

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    Phase & aPhase,
                                    const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity pill = *handle.get(aPhase);
    addMeshGeoNode(aContext, pill, "models/burger/burger.gltf",
                   "effects/MeshTextures.sefx",
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gPillHeight},
                   0.16f, {1.f, 1.f, 1.f},
                   math::Quaternion<float>{
                       math::UnitVec<3, float>{{1.f, 0.f, 0.f}}, Turn_f{0.1f}});
    pill
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::Pill{})
        .add(component::Collision{component::gPillHitbox})
        .add(component::LevelEntity{});
    return handle;
}

ent::Handle<ent::Entity>
createPowerUp(GameContext & aContext,
              Phase & aPhase,
              const math::Position<2, float> & aGridPos,
              const component::PowerUpType aType,
              float aSwapPeriod)
{
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(aPhase);
    component::PowerUpBaseInfo info =
        component::gPowerupInfoByType.at(static_cast<unsigned int>(aType));
    addMeshGeoNode(aContext, powerUp, info.mPath, "effects/MeshTextures.sefx",
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gPillHeight},
                   info.mLevelScaling, info.mLevelInstanceScale,
                   component::gLevelBasePowerupQuat * info.mLevelOrientation);
    powerUp
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::PowerUp{
            .mType = aType,
            .mSwapPeriod = aSwapPeriod,
        })
        .add(component::Collision{component::gPowerUpHitbox})
        .add(component::LevelEntity{});
    return handle;
}

EntHandle createPlayerPowerUp(GameContext & aContext,
                              const component::PowerUpType aType)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(init);
    component::PowerUpBaseInfo info =
        component::gPowerupInfoByType.at(static_cast<unsigned int>(aType));
    addMeshGeoNode(
        aContext, powerUp, info.mPlayerPath, "effects/MeshTextures.sefx",
        Pos3{1.f, 1.f, 0.f} + info.mPlayerPosOffset, info.mPlayerScaling,
        info.mPlayerInstanceScale, info.mPlayerOrientation);
    powerUp.add(component::LevelEntity{});
    return handle;
}

EntHandle createPathEntity(GameContext & aContext,
                           Phase & aPhase,
                           const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gWhite<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   Phase & aPhase,
                   const math::Position<2, float> & aGridPos,
                   int aPortalIndex)
{
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gRed<float>);
    Entity portal = *handle.get(aPhase);
    portal.add(component::Portal{aPortalIndex});
    return handle;
}

void addPortalInfo(GameContext & aContext,
                   component::Portal & aPortal,
                   const component::Geometry & aGeo,
                   Vec3 aDirection)
{
    Box_f portalEntrance{component::gPortalHitbox};
    portalEntrance.mPosition += aDirection;
    Box_f portalExit{component::gPortalHitbox};
    portalExit.mPosition -= aDirection * 0.8f;

    Phase addHitboxPhase;
    aPortal.mEnterHitbox = portalEntrance;
    aPortal.mExitHitbox = portalExit;
    aPortal.mMirrorSpawnPosition = aGeo.mPosition + aDirection;

    auto model = aContext.mWorld.addEntity();
    {
        Phase portalModelPhase;
        Entity modelEnt = *model.get(portalModelPhase);
        addMeshGeoNode(
            aContext, modelEnt, "models/portal/portal.gltf",
            "effects/MeshTextures.sefx",
            {aGeo.mPosition.x() + aDirection.x() * 1.3f, aGeo.mPosition.y(),
             0.f},
            0.2f, {1.f, 1.f, 1.f},
            Quat_f{UnitVec3::MakeFromUnitLength({0.f, 0.f, 1.f}),
                   Turn_f{(-aDirection.x() - 1.f) / 2.f * 0.5f}}
                * Quat_f{UnitVec3::MakeFromUnitLength({1.f, 0.f, 0.f}),
                         Turn_f{0.25f}});
        modelEnt.add(component::LevelEntity{});
    }
    model.get()->get<component::SceneNode>().mName = "portal";
    insertEntityInScene(model, *aContext.mLevel);
}

ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   Phase & aPhase,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gYellow<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        Phase & aPhase,
                        const math::Position<2, float> & aGridPos)
{
    math::Position<3, float> spawnPos = {static_cast<float>(aGridPos.x()),
                                         static_cast<float>(aGridPos.y()),
                                         gPlayerHeight};
    auto spawner = aContext.mWorld.addEntity();
    createLevelElement(aPhase, spawner, aContext, aGridPos,
                       math::hdr::gCyan<float>);
    spawner.get(aPhase)->add(component::Spawner{.mSpawnPosition = spawnPos});

    return spawner;
}

ent::Handle<ent::Entity>
createHudBillpad(GameContext & aContext,
                 const component::PlayerSlot & aPlayerSlot,
                 const component::PlayerLifeCycle & aPlayerLifeCycle)
{
    EntHandle hudHandle = aContext.mWorld.addEntity();
    {
        Phase createHud;

        ent::Entity hud = *hudHandle.get(createHud);
        // Create the Hud common ancestor in the scene graph
        addGeoNode(aContext, hud,
                    // TODO bake the -7 -7 into the hud positions.
                   component::gHudPositionsWorld[aPlayerSlot.mIndex] + Vec3{-7.f, -7.f, 0.f},
                   1.f,
                   {1.f, 1.f, 1.f},
                   component::gHudOrientationsWorld[aPlayerSlot.mIndex]);
    }

    // The score text line
    EntHandle scoreHandle = aContext.mWorld.addEntity();
    EntHandle roundHandle = aContext.mWorld.addEntity();
    EntHandle powerupHandle = aContext.mWorld.addEntity();
    EntHandle billpadHandle = aContext.mWorld.addEntity();
    {
        Phase createScore;

        const std::string fontname = "fonts/notes/Bitcheese.ttf";

        // Score
        {
            ent::Entity scoreText = *scoreHandle.get(createScore);
            scoreText.add(component::Text{
                .mString = std::to_string(aPlayerLifeCycle.mScore),
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });

            addGeoNode(aContext, scoreText, {-1.7f, 0.6f, 0.f}, 1.f);
        }

        // Rounds
        {
            ent::Entity roundText = *roundHandle.get(createScore);
            roundText.add(component::Text{
                .mString = std::to_string(aPlayerLifeCycle.mRoundsWon),
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });

            addGeoNode(aContext, roundText, {0.5f, 0.3f, 0.f}, 1.1f);
        }

        // Power-up
        {
            ent::Entity powerupText = *powerupHandle.get(createScore);
            powerupText.add(component::Text{
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });
            addGeoNode(aContext, powerupText, {-1.9f, -.7f, 0.f}, 0.5f);
        }

        // Billpad model
        {
            ent::Entity billpad = *billpadHandle.get(createScore);
            addMeshGeoNode(
                aContext, billpad, "models/billpad/billpad.gltf",
                "effects/MeshTextures.sefx", {0.f, 0.f, -0.30f}, 8.f,
                {1.f, 1.f, 1.f},
                Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                       math::Turn<float>{0.25f}}
                    * Quat_f{math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                             math::Turn<float>{-0.25f}},
                aPlayerSlot.mColor);
        }
    }

    {
        Phase completeSceneGraph;

        insertEntityInScene(scoreHandle, hudHandle);
        insertEntityInScene(roundHandle, hudHandle);
        insertEntityInScene(powerupHandle, hudHandle);
        insertEntityInScene(billpadHandle, hudHandle);

        assert(aContext.mLevel);
        // Insert the billpad into the root, not the level,
        // because the level scale changes depending on the tile dimensions.
        insertEntityInScene(hudHandle, *aContext.mRoot);

        hudHandle.get(completeSceneGraph)
            ->add(component::PlayerHud{
                .mScoreText = scoreHandle,
                .mRoundText = roundHandle,
                .mPowerupText = powerupHandle,
            });
    }

    return hudHandle;
}

ent::Handle<ent::Entity> fillSlotWithPlayer(GameContext & aContext,
                                            ControllerType aControllerType,
                                            ent::Handle<ent::Entity> aSlot,
                                            int aControllerId)
{
    auto font =
        aContext.mResources.getFont("fonts/FredokaOne-Regular.ttf", 120);

    EntHandle playerModel = aContext.mWorld.addEntity();

    {
        Phase sceneInit;
        Entity player = *aSlot.get(sceneInit);

        component::PlayerSlot & playerSlot =
            player.get<component::PlayerSlot>();

        Entity model = *playerModel.get(sceneInit);

        addGeoNode(aContext, player, {0.f, 0.f, gPlayerHeight});
        std::shared_ptr<snac::Model> modelData = addMeshGeoNode(
            aContext, model, "models/donut/donut.gltf",
            "effects/MeshRiggingTextures.sefx", {0.f, 0.f, 0.f}, 1.f,
            {0.2f, 0.2f, 0.2f},
            Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                   math::Turn<float>{0.25f}}
                * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                         math::Turn<float>{0.25f}},
            playerSlot.mColor);
        std::string animName = "idle";
        const snac::NodeAnimation & animation =
            modelData->mAnimations.at(animName);
        model.add(component::RigAnimation{
            .mAnimName = animName,
            .mAnimation = &animation,
            .mAnimationMap = &modelData->mAnimations,
            .mStartTime = snac::Clock::now(),
            .mParameter =
                decltype(component::RigAnimation::mParameter){
                    animation.mEndTime},
        });
    }

    insertEntityInScene(playerModel, aSlot);

    {
        Phase createPlayer;
        Entity player = *aSlot.get(createPlayer);

        component::PlayerSlot & playerSlot =
            player.get<component::PlayerSlot>();
        playerSlot.mFilled = true;

        player.add(component::PlayerModel{.mModel = playerModel})
            .add(component::PlayerLifeCycle{
                .mIsAlive = false,
                .mTimeToRespawn = component::gBaseTimeToRespawn,
            })
            .add(component::PlayerMoveState{})
            .add(component::AllowedMovement{})
            .add(component::Controller{.mType = aControllerType,
                                       .mControllerId = aControllerId})
            .add(component::Collision{component::gPlayerHitbox})
            .add(component::PlayerPortalData{});
    }
    return aSlot;
}

std::optional<ent::Handle<ent::Entity>>
findSlotAndBind(GameContext & aContext,
                ent::Query<component::PlayerSlot> & aSlots,
                ControllerType aType,
                int aIndex)
{
    std::optional<ent::Handle<ent::Entity>> freeSlot = snac::getFirstHandle(
        aSlots, [](component::PlayerSlot & aSlot) { return !aSlot.mFilled; });

    if (freeSlot)
    {
        return fillSlotWithPlayer(aContext, aType, *freeSlot, aIndex);
    }

    return std::nullopt;
}

ent::Handle<ent::Entity>
createMenuItem(GameContext & aContext,
               ent::Phase & aInit,
               const std::string & aString,
               std::shared_ptr<snac::Font> aFont,
               const math::Position<2, float> & aPos,
               const std::unordered_map<int, std::string> & aNeighbors,
               const scene::Transition & aTransition,
               bool aSelected,
               const math::Size<2, float> & aScale)
{
    auto item = makeText(
        aContext, aInit, aString, aFont,
        aSelected ? scene::gColorItemSelected : scene::gColorItemUnselected,
        aPos, aScale);
    item.get(aInit)->add(component::MenuItem{.mName = aString,
                                             .mSelected = aSelected,
                                             .mNeighbors = aNeighbors,
                                             .mTransition = aTransition});

    return item;
}

ent::Handle<ent::Entity>
makeText(GameContext & aContext,
         ent::Phase & aPhase,
         const std::string & aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale)
{
    auto handle = aContext.mWorld.addEntity();

    handle.get(aPhase)
        ->add(component::Text{
            .mString{aString},
            .mFont = std::move(aFont),
            .mColor = aColor,
        })
        .add(component::PoseScreenSpace{.mPosition_u = aPosition_unitscreen,
                                        .mScale = aScale});

    return handle;
}

void swapPlayerPosition(Phase & aPhase, EntHandle aPlayer, EntHandle aOther)
{
    component::Geometry & aGeo =
        aPlayer.get(aPhase)->get<component::Geometry>();
    component::Geometry & aOtherGeo =
        aOther.get(aPhase)->get<component::Geometry>();
    Pos3 temp = aGeo.mPosition;
    aGeo.mPosition = aOtherGeo.mPosition;
    aOtherGeo.mPosition = temp;

    // Remove portal image if there is one
}

void removeRoundTransientPlayerComponent(Phase & aPhase, EntHandle aHandle)
{
    Entity playerEntity = *aHandle.get(aPhase);
    if (playerEntity.has<component::PlayerPowerUp>())
    {
        component::PlayerPowerUp powerup =
            playerEntity.get<component::PlayerPowerUp>();
        Entity powerupEntity = *powerup.mPowerUp.get(aPhase);

        switch (powerup.mType)
        {
        case component::PowerUpType::Dog:
        case component::PowerUpType::Missile:
            break;
        case component::PowerUpType::Teleport:
            if (std::get<component::TeleportPowerUpInfo>(powerup.mInfo)
                    .mTargetArrow)
            {
                std::get<component::TeleportPowerUpInfo>(powerup.mInfo)
                    .mTargetArrow->get(aPhase)
                    ->erase();
            }
            break;
        case component::PowerUpType::_End:
            break;
        }

        powerupEntity.erase();
        playerEntity.remove<component::PlayerPowerUp>();
    }

    if (playerEntity.get<component::PlayerPortalData>().mPortalImage)
    {
        playerEntity.get<component::PlayerPortalData>()
            .mPortalImage->get(aPhase)
            ->erase();
        playerEntity.get<component::PlayerPortalData>().mCurrentPortal = -1;
        playerEntity.get<component::PlayerPortalData>().mDestinationPortal = -1;
    }

    // Remove the whole hud subtree (billpad, texts, ...)
    assert(playerEntity.has<component::PlayerLifeCycle>());
    auto & lifeCycle = playerEntity.get<component::PlayerLifeCycle>();
    eraseEntityRecursive(*lifeCycle.mHud, aPhase);
    lifeCycle.mHud = std::nullopt;
}

EntHandle removePlayerFromGame(Phase & aPhase, EntHandle aHandle)
{
    component::PlayerSlot & slot =
        aHandle.get(aPhase)->get<component::PlayerSlot>();
    slot.mFilled = false;

    removeRoundTransientPlayerComponent(aPhase, aHandle);

    Entity playerEntity = *aHandle.get(aPhase);

    playerEntity.remove<component::Controller>();
    playerEntity.remove<component::PlayerLifeCycle>();
    playerEntity.remove<component::PlayerMoveState>();
    playerEntity.remove<component::VisualModel>();
    playerEntity.get<component::PlayerModel>().mModel.get(aPhase)->erase();
    playerEntity.remove<component::PlayerModel>();
    playerEntity.remove<component::Geometry>();
    playerEntity.remove<component::GlobalPose>();
    playerEntity.remove<component::PlayerPortalData>();
    playerEntity.remove<component::SceneNode>();

    return aHandle;
}

EntHandle createStageDecor(GameContext & aContext)
{
    EntHandle result = aContext.mWorld.addEntity();
    {
        Phase createStage;
        Entity stageEntity = *result.get(createStage);

        addMeshGeoNode(aContext, stageEntity, "models/stage/stage.gltf",
                       "effects/MeshTextures.sefx", {0.f, 0.f, -0.4f}, 
                       1.0f,
                       {1.1f, 1.1f, 1.f},
                       Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                              math::Turn<float>{0.25f}}
                           * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                    math::Turn<float>{0.25f}});
    }
    return result;
}

EntHandle createTargetArrow(GameContext & aContext, const HdrColor_f & aColor)
{
    EntHandle result = aContext.mWorld.addEntity();
    {
        Phase createTargetArrow;
        Entity stageEntity = *result.get(createTargetArrow);

        addMeshGeoNode(aContext, stageEntity, "models/arrow/arrow.gltf",
                       "effects/MeshTextures.sefx", {0.f, 0.f, 2.f}, 0.4f,
                       {1.f, 1.f, 1.f},
                       Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                              math::Turn<float>{0.25f}},
                       aColor);

        stageEntity.add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}}});
    }
    return result;
}

EntHandle createExplosion(GameContext & aContext,
                          const math::Position<3, float> & aPos,
                          const snac::Time & aTime)
{
    EntHandle explosion = aContext.mWorld.addEntity();
    {
        Phase createExplosion;
        Entity explosionEnt = *explosion.get(createExplosion);
        addMeshGeoNode(aContext, explosionEnt, "models/boom/boom.gltf",
                       "effects/MeshTextures.sefx", aPos, 0.1f);
        explosionEnt
            .add(component::Explosion{
                .mStartTime = aTime.mTimepoint,
                .mParameter = math::ParameterAnimation<
                    float, math::AnimationResult::Clamp>(0.5f)})
            .add(component::LevelEntity{});
    }
    return explosion;
}

EntHandle createPortalImage(GameContext & aContext,
                            component::PlayerModel & aPlayerModel,
                            const component::Portal & aPortal,
                            component::PlayerPortalData & aPortalData)
{
    EntHandle newPortalImage = aContext.mWorld.addEntity();
    {
        Phase addPortalImage;
        component::Geometry modelGeo =
            aPlayerModel.mModel.get()->get<component::Geometry>();
        component::RigAnimation animation =
            aPlayerModel.mModel.get()->get<component::RigAnimation>();
        Entity portal = *newPortalImage.get(addPortalImage);
        Vec2 relativePos =
            aPortal.mMirrorSpawnPosition.xy() - aPortalData.mCurrentPortalPos;
        addMeshGeoNode(aContext, portal, "models/donut/donut.gltf",
                       "effects/MeshTextures.sefx",
                       {relativePos.x(), relativePos.y(), 0.f},
                       modelGeo.mScaling, modelGeo.mInstanceScaling,
                       modelGeo.mOrientation, modelGeo.mColor);
        portal.add(component::Collision{component::gPlayerHitbox})
            .add(component::RigAnimation{
                .mAnimName = animation.mAnimName,
                .mAnimation = animation.mAnimation,
                .mStartTime = animation.mStartTime,
                .mParameter =
                    decltype(component::RigAnimation::mParameter){
                        animation.mAnimation->mEndTime},
            });
        aPortalData.mPortalImage = newPortalImage;
    }

    return newPortalImage;
}

} // namespace snacgame
} // namespace ad
