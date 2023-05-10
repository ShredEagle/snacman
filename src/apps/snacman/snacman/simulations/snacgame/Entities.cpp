#include "Entities.h"

#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/LevelTags.h"
#include "component/MenuItem.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/RigAnimation.h"
#include "component/SceneNode.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"
#include "component/VisualModel.h"
#include "GameContext.h"
#include "scene/MenuScene.h"
#include "component/AllowedMovement.h"
#include "component/PlayerPowerUp.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/component/PlayerPortalData.h"
#include "typedef.h"

#include "../../QueryManipulation.h"
#include "../../Resources.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

#include <algorithm>
#include <map>
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

std::shared_ptr<snac::Model> 
addMeshGeoNode(Phase & aPhase,
               GameContext & aContext,
               Entity & aEnt,
               const char * aModelPath,
               Pos3 aPos,
               float aScale,
               Size3 aInstanceScale,
               Quat_f aOrientation,
               HdrColor_f aColor)
{
    auto model = aContext.mResources.getModel(aModelPath);
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
    addMeshGeoNode(aPhase, aContext, path, "models/square_biscuit/square_biscuit.gltf",
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gLevelHeight},
                   0.45f, lLevelElementScaling, math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}}, aColor);
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

    auto font = aContext.mResources.getFont("fonts/FredokaOne-Regular.ttf", 120);

    text.add(component::Text{
            .mString{std::move(aText)},
            .mFont = std::move(font),
            .mColor = math::hdr::gYellow<float>,
        })
        .add(aPose)
        .add(component::LevelEntity{})
        ;

    return handle;
}


ent::Handle<ent::Entity> createAnimatedTest(GameContext & aContext,
                                            Phase & aPhase,
                                            snac::Clock::time_point aStartTime,
                                            const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity entity = *handle.get(aPhase);
    std::shared_ptr<snac::Model> model = 
        addMeshGeoNode(aPhase, aContext, entity, "models/anim/anim.gltf",
                       {
                            static_cast<float>(aGridPos.x()),
                            static_cast<float>(aGridPos.y()),
                            gLevelHeight
                       },
                       0.45f,
                       lLevelElementScaling,
                       math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                       math::Turn<float>{0.25f}});
    const snac::NodeAnimation & animation = model->mAnimations.begin()->second;
    entity.add(component::RigAnimation{
        .mAnimation = &animation,
        .mStartTime = aStartTime,
        .mParameter{animation.mEndTime},
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
    addMeshGeoNode(
        aPhase, aContext, pill, "models/burger/burger.gltf",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gPillHeight},
        0.16f, {1.f, 1.f, 1.f},
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                Turn_f{0.1f}});
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

ent::Handle<ent::Entity> createPowerUp(GameContext & aContext,
        Phase & aPhase,
                                    const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(aPhase);
    addMeshGeoNode(
        aPhase, aContext, powerUp, "models/collar/collar.gltf",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gPillHeight},
        0.3f, {1.f, 1.f, 1.f},
        Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                Turn_f{0.25f}});
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

EntHandle createPlayerPowerUp(GameContext & aContext)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(init);
    addMeshGeoNode(
        init, aContext, powerUp, "models/collar/collar.gltf",
        {0.f, -1.f, 1.5f},
        0.3f, {1.f, 1.f, 1.f});
    return handle;
}

EntHandle
createPathEntity(GameContext & aContext,
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
                   const math::Position<2, float> & aGridPos, int aPortalIndex)
{
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gRed<float>);
    Entity portal = *handle.get(aPhase);
    portal.add(component::Portal{aPortalIndex});
    return handle;
}

void addPortalInfo(component::Portal & aPortal, const component::Geometry & aGeo, Vec3 aDirection)
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

        addGeoNode(
            sceneInit, aContext, player,
            {0.f, 0.f, gPlayerHeight});
        addMeshGeoNode(
            sceneInit, aContext, model, "models/donut/donut.gltf",
            {0.f, 0.f, 0.f}, 0.2f, {1.f, 1.f, 1.f},
            Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                                    math::Turn<float>{0.25f}} * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
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
        std::ostringstream playerText;
        playerText << "P" << playerSlot.mIndex + 1 << " 0";

        player
            .add(component::PlayerModel{.mModel = playerModel})
            .add(component::PlayerLifeCycle{.mIsAlive = false})
            .add(component::PlayerMoveState{})
            .add(component::AllowedMovement{})
            .add(component::Controller{.mType = aControllerType,
                                       .mControllerId = aControllerId})
            .add(component::Text{
                .mString{playerText.str()},
                .mFont = std::move(font),
                .mColor = playerSlot.mColor,
            })
            .add(component::PoseScreenSpace{
                .mPosition_u = {-0.9f + 0.2f * static_cast<float>(playerSlot.mIndex), 0.8f}})
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
        return fillSlotWithPlayer(aContext, aType, *freeSlot,
                                  aIndex);
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

EntHandle removePlayerFromGame(Phase & aPhase, EntHandle aHandle)
{
    component::PlayerSlot & slot = aHandle.get(aPhase)->get<component::PlayerSlot>();
    slot.mFilled = false;

    aHandle.get(aPhase)->remove<component::Controller>();
    aHandle.get(aPhase)->remove<component::Geometry>();
    aHandle.get(aPhase)->remove<component::PlayerLifeCycle>();
    aHandle.get(aPhase)->remove<component::PlayerMoveState>();
    aHandle.get(aPhase)->remove<component::Text>();
    aHandle.get(aPhase)->remove<component::VisualModel>();
    aHandle.get(aPhase)->remove<component::PoseScreenSpace>();
    aHandle.get(aPhase)->remove<component::SceneNode>();
    aHandle.get(aPhase)->remove<component::GlobalPose>();
    if (aHandle.get(aPhase)->has<component::PlayerPowerUp>())
    {
        aHandle.get(aPhase)->get<component::PlayerPowerUp>().mPowerUp.get(aPhase)->erase();
        aHandle.get(aPhase)->remove<component::PlayerPowerUp>();
    }

    if (aHandle.get(aPhase)->get<component::PlayerPortalData>().mPortalImage)
    {
        aHandle.get(aPhase)->get<component::PlayerPortalData>().mPortalImage->get(aPhase)->erase();
        aHandle.get(aPhase)->remove<component::PlayerPortalData>();
    }

    aHandle.get(aPhase)->get<component::PlayerModel>().mModel.get(aPhase)->erase();
    aHandle.get(aPhase)->remove<component::PlayerModel>();


    return aHandle;
}

EntHandle createStageDecor(GameContext & aContext)
{
    EntHandle result = aContext.mWorld.addEntity();
    {
        Phase createStage;
        Entity stageEntity = *result.get(createStage);

        addMeshGeoNode(createStage, aContext, stageEntity, "models/stage/stage.gltf",
                {7.f, 7.f, -0.4f}, 1.f, {1.f, 1.f, 1.f},
                Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                                        math::Turn<float>{0.25f}} * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                        math::Turn<float>{0.25f}}
                );
        stageEntity.add(component::LevelEntity{});
    }
    return result;
}

} // namespace snacgame
} // namespace ad
