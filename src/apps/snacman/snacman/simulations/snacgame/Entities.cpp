#include "Entities.h"

#include "component/AllowedMovement.h"
#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/LevelTags.h"
#include "component/MenuItem.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerPowerUp.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/PowerUp.h"
#include "component/SceneNode.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"
#include "component/VisualModel.h"
#include "GameContext.h"
#include "scene/MenuScene.h"
#include "snacman/simulations/snacgame/component/PlayerHud.h"
#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/PlayerPortalData.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "typedef.h"

#include "../../QueryManipulation.h"
#include "../../Resources.h"

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

void addGeoNode(Phase & aPhase,
                GameContext & aContext,
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

void addMeshGeoNode(Phase & aPhase,
                    GameContext & aContext,
                    Entity & aEnt,
                    const char * aModelPath,
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
        .add(component::VisualModel{
            .mModel = aContext.mResources.getModel(aModelPath),
        })
        .add(component::SceneNode{})
        .add(component::GlobalPose{});
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
        aPhase, aContext, path, "models/square_biscuit/square_biscuit.gltf",
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

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    Phase & aPhase,
                                    const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity pill = *handle.get(aPhase);
    addMeshGeoNode(aPhase, aContext, pill, "models/burger/burger.gltf",
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
              const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(aPhase);
    component::PowerUpType baseType = component::PowerUpType::Dog;
    component::PowerUpBaseInfo info = component::gPowerupPathByType.at(static_cast<unsigned int>(baseType));
    addMeshGeoNode(
        aPhase, aContext, powerUp, info.mPath,
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gPillHeight},
        info.mLevelScaling, info.mLevelInstanceScale,
        component::gLevelBasePowerupQuat * info.mLevelOrientation);
    powerUp
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::PowerUp{.mType = component::PowerUpType::Dog})
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
    component::PowerUpBaseInfo info = component::gPowerupPathByType.at(static_cast<unsigned int>(aType));
    addMeshGeoNode(
        init, aContext, powerUp,
        info.mPath,
        {1.f, 1.f, 0.f}, info.mPlayerScaling, info.mPlayerInstanceScale, info.mPlayerOrientation);
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

void addPortalInfo(component::Portal & aPortal,
                   const component::Geometry & aGeo,
                   Vec3 aDirection)
{
    Box_f portalEntrance{component::gPortalHitbox};
    portalEntrance.mPosition += aDirection;
    Box_f portalExit{component::gPortalHitbox};
    portalExit.mPosition -= aDirection;

    Phase addHitboxPhase;
    aPortal.mEnterHitbox = portalEntrance;
    aPortal.mExitHitbox = portalExit;
    aPortal.mMirrorSpawnPosition = aGeo.mPosition + aDirection;
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

        addGeoNode(sceneInit, aContext, player, {0.f, 0.f, gPlayerHeight});
        addMeshGeoNode(sceneInit, aContext, model, "models/donut/donut.gltf",
                       {0.f, 0.f, 0.f}, 1.f, {0.2f, 0.2f, 0.2f},
                       Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                              math::Turn<float>{0.25f}}
                           * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                    math::Turn<float>{0.25f}},
                       playerSlot.mColor);
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
                .mTimeToRespawn = component::gBaseTimeToRespawn})
            .add(component::PlayerMoveState{})
            .add(component::AllowedMovement{})
            .add(component::Controller{.mType = aControllerType,
                                       .mControllerId = aControllerId})
            .add(component::PoseScreenSpace{
                .mPosition_u =
                    {-0.9f + 0.2f * static_cast<float>(playerSlot.mIndex),
                     0.8f}})
            .add(component::Collision{component::gPlayerHitbox})
            .add(component::PlayerPortalData{})
            .add(component::PlayerHud{});
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
    component::Geometry & aGeo = aPlayer.get(aPhase)->get<component::Geometry>();
    component::Geometry & aOtherGeo = aOther.get(aPhase)->get<component::Geometry>();
    Pos3 temp = aGeo.mPosition;
    aGeo.mPosition = aOtherGeo.mPosition;
    aOtherGeo.mPosition = temp;
    
    //Remove portal image if there is one
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
        aHandle.get(aPhase)->remove<component::PlayerPortalData>();
    }
}

EntHandle removePlayerFromGame(Phase & aPhase, EntHandle aHandle)
{
    component::PlayerSlot & slot =
        aHandle.get(aPhase)->get<component::PlayerSlot>();
    slot.mFilled = false;

    Entity playerEntity = *aHandle.get(aPhase);
    playerEntity.remove<component::Controller>();
    playerEntity.remove<component::Geometry>();
    playerEntity.remove<component::PlayerLifeCycle>();
    playerEntity.remove<component::PlayerMoveState>();
    playerEntity.remove<component::Text>();
    playerEntity.remove<component::VisualModel>();
    playerEntity.remove<component::PoseScreenSpace>();
    playerEntity.remove<component::SceneNode>();
    playerEntity.remove<component::GlobalPose>();
    playerEntity.remove<component::PlayerHud>();

    removeRoundTransientPlayerComponent(aPhase, aHandle);

    playerEntity.get<component::PlayerModel>().mModel.get(aPhase)->erase();
    playerEntity.remove<component::PlayerModel>();

    return aHandle;
}

EntHandle createStageDecor(GameContext & aContext)
{
    EntHandle result = aContext.mWorld.addEntity();
    {
        Phase createStage;
        Entity stageEntity = *result.get(createStage);

        addMeshGeoNode(createStage, aContext, stageEntity,
                       "models/stage/stage.gltf", {7.f, 7.f, -0.4f}, 1.f,
                       {1.f, 1.f, 1.f},
                       Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                              math::Turn<float>{0.25f}}
                           * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                    math::Turn<float>{0.25f}});
        stageEntity.add(component::LevelEntity{});
    }
    return result;
}

EntHandle createTargetArrow(GameContext & aContext, const HdrColor_f & aColor)
{
    EntHandle result = aContext.mWorld.addEntity();
    {
        Phase createTargetArrow;
        Entity stageEntity = *result.get(createTargetArrow);

        addMeshGeoNode(createTargetArrow, aContext, stageEntity,
                       "models/arrow/arrow.gltf", {0.f, 0.f, 2.f}, 0.4f,
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

} // namespace snacgame
} // namespace ad
