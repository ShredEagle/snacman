#include "Entities.h"

#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/LevelTags.h"
#include "component/MenuItem.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PlayerSlot.h"
#include "component/PoseScreenSpace.h"
#include "component/SceneNode.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"
#include "component/VisualMesh.h"
#include "GameContext.h"
#include "scene/MenuScene.h"
#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/PlayerPowerUp.h"
#include "typedef.h"

#include "../../QueryManipulation.h"
#include "../../Resources.h"

#include <algorithm> // for max
#include <entity/Archetype.h>
#include <entity/detail/HandledStore.h>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <entity/QueryStore.h>
#include <map> // for opera...
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>
#include <optional> // for optional
#include <ostream>  // for opera...
#include <tuple>    // for get
#include <unordered_map>
#include <utility> // for move
#include <vector>  // for vector

namespace ad {
namespace snacgame {

constexpr Size3 lLevelElementScaling = {1.f, 1.f, 1.f};

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
    addMeshGeoNode(aPhase, aContext, path, "models/floor/floor.gltf",
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gLevelHeight},
                   0.5f, lLevelElementScaling, math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}}, aColor);
    path.add(component::LevelEntity{});
}
} // namespace

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    const math::Position<2, float> & aGridPos)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    Entity pill = *handle.get(init);
    addMeshGeoNode(
        init, aContext, pill, "models/burger/burger.gltf",
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
                                    const math::Position<2, float> & aGridPos)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    Entity powerUp = *handle.get(init);
    addMeshGeoNode(
        init, aContext, powerUp, "models/burger/burger.gltf",
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
        init, aContext, powerUp, "models/burger/burger.gltf",
        {0.f, 1.5f, -1.f},
        0.3f, {1.f, 1.f, 1.f});
    return handle;
}

ent::Handle<ent::Entity>
createPathEntity(GameContext & aContext,
                 const math::Position<2, float> & aGridPos)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(init, handle, aContext, aGridPos,
                       math::hdr::gWhite<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   const math::Position<2, float> & aGridPos, int aPortalIndex)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(init, handle, aContext, aGridPos,
                       math::hdr::gRed<float>);
    handle.get(init)->add(component::Portal{aPortalIndex});
    return handle;
}

ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   const math::Position<2, float> & aGridPos)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity();
    createLevelElement(init, handle, aContext, aGridPos,
                       math::hdr::gYellow<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        const math::Position<2, float> & aGridPos)
{
    ent::Phase init;
    math::Position<3, float> spawnPos = {static_cast<float>(aGridPos.x()),
                                         static_cast<float>(aGridPos.y()),
                                         gPlayerHeight};
    auto spawner = aContext.mWorld.addEntity();
    createLevelElement(init, spawner, aContext, aGridPos,
                       math::hdr::gCyan<float>);
    spawner.get(init)->add(component::Spawner{.mSpawnPosition = spawnPos});

    return spawner;
}

ent::Handle<ent::Entity> fillSlotWithPlayer(GameContext & aContext,
                                            ent::Phase & aInit,
                                            ControllerType aControllerType,
                                            ent::Handle<ent::Entity> aSlot,
                                            int aControllerId)
{
    auto font =
        aContext.mResources.getFont("fonts/FredokaOne-Regular.ttf", 120);

    Entity player = *aSlot.get(aInit);
    component::PlayerSlot & playerSlot =
        player.get<component::PlayerSlot>();
    playerSlot.mFilled = true;

    std::ostringstream playerText;
    playerText << "P" << playerSlot.mIndex + 1 << " 0";

    addMeshGeoNode(
        aInit, aContext, player, "models/donut/DONUT.gltf",
        {0.f, 0.f, gPlayerHeight}, 1.f, {1.f, 1.f, 1.f},
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}},
        playerSlot.mColor);

    player
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
        .add(component::Collision{component::gPlayerHitbox});

    return aSlot;
}

std::optional<ent::Handle<ent::Entity>>
findSlotAndBind(GameContext & aContext,
                ent::Phase & aBindPhase,
                ent::Query<component::PlayerSlot> & aSlots,
                ControllerType aType,
                int aIndex)
{
    std::optional<ent::Handle<ent::Entity>> freeSlot = snac::getFirstHandle(
        aSlots, [](component::PlayerSlot & aSlot) { return !aSlot.mFilled; });

    if (freeSlot)
    {
        return fillSlotWithPlayer(aContext, aBindPhase, aType, *freeSlot,
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

    return aHandle;
}

} // namespace snacgame
} // namespace ad
