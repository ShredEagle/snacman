#include "Entities.h"

#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/LevelData.h"
#include "component/MenuItem.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PoseScreenSpace.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"
#include "component/VisualMesh.h"
#include "GameContext.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/PlayerSlot.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"

#include "../../QueryManipulation.h"

#include <cstdlib>
#include <math/Color.h>
#include <snac-renderer/text/Text.h>
#include <unordered_map>

namespace ad {
namespace snacgame {

void createPill(GameContext & aContext,
                ent::Phase & aInit,
                const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    handle.get(aInit)
        ->add(component::Geometry{
            .mPosition = {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y())},
            .mScaling = math::Size<3, float>{15.f, 15.f, 15.f},
            .mLayer = component::GeometryLayer::Player,
            .mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                                                        {0.f, 1.f, 0.f}},
                                                    math::Degree<float>{15.f}},
            .mColor = math::hdr::Rgb<float>{1.f, 0.5f, 0.5f}})
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::VisualMesh{
            .mMesh =
                aContext.mResources.getShape("models/avocado/Avocado.gltf"),
        })
        .add(component::Pill{})
        .add(component::Collision{component::gPillHitbox})
        .add(component::LevelEntity{});
    ;
}

ent::Handle<ent::Entity>
createPathEntity(GameContext & aContext,
                 ent::Phase & aInit,
                 const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    handle.get(aInit)
        ->add(component::LevelEntity{})
        .add(component::Geometry{
            .mPosition = {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y())},
            .mScaling = math::Size<3, float>{0.5f, 0.5f, 0.5f},
            .mLayer = component::GeometryLayer::Level,
            .mColor = math::hdr::gWhite<float>})
        .add(component::VisualMesh{
            .mMesh = aContext.mResources.getShape("CUBE"),
        });
    ;
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   ent::Phase & aInit,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)
        ->add(component::Geometry{
            .mPosition = {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y())},
            .mScaling = math::Size<3, float>{0.5f, 0.5f, 0.5f},
            .mLayer = component::GeometryLayer::Level,
            .mColor = math::hdr::gRed<float>})
        .add(component::VisualMesh{
            .mMesh = aContext.mResources.getShape("CUBE"),
        });
    return handle;
}

ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   ent::Phase & aInit,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)
        ->add(component::Geometry{
            .mPosition = {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y())},
            .mScaling = math::Size<3, float>{0.5f, 0.5f, 0.5f},
            .mLayer = component::GeometryLayer::Level,
            .mColor = math::hdr::gYellow<float>})
        .add(component::VisualMesh{
            .mMesh = aContext.mResources.getShape("CUBE"),
        });
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        ent::Phase & aInit,
                        const math::Position<2, float> & aGridPos)
{
    math::Position<2, float> gridFloatPos = {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y())};
    auto spawner = aContext.mWorld.addEntity();
    spawner.get(aInit)->add(component::LevelEntity{});
    spawner.get(aInit)
        ->add(component::Geometry{
            .mPosition = gridFloatPos,
            .mScaling = math::Size<3, float>{0.5f, 0.5f, 0.5f},
            .mLayer = component::GeometryLayer::Level,
            .mColor = math::hdr::gCyan<float>})
        .add(component::VisualMesh{
            .mMesh = aContext.mResources.getShape("CUBE"),
        });
    spawner.get(aInit)->add(component::Spawner{.mSpawnPosition = gridFloatPos});

    return spawner;
}

void fillSlotWithPlayer(GameContext & aContext,
                        ent::Phase & aInit,
                        ControllerType aControllerType,
                        ent::Handle<ent::Entity> aSlot,
                        int aControllerId)
{
    auto font =
        aContext.mResources
            .getFont("fonts/FredokaOne-Regular.ttf", 120);

    auto playerEntity = aSlot.get(aInit);
    const component::PlayerSlot & playerSlot = playerEntity->get<component::PlayerSlot>();

    std::ostringstream playerText;
    playerText << "P" << playerSlot.mIndex + 1;

    playerEntity->add(component::Geometry{
        .mPosition = math::Position<2, float>::Zero(),
        .mScaling = {50.f, 50.f, 50.f},
        .mLayer = component::GeometryLayer::Player,
        .mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{
                                                    {0.f, 1.f, 0.f}},
                                                math::Degree<float>{-90.f}},
        .mColor = playerSlot.mColor
    })
        .add(component::PlayerLifeCycle{.mIsAlive = false})
        .add(component::PlayerMoveState{})
        .add(component::Controller{
            .mType = aControllerType,
            .mControllerId = aControllerId
        })
        .add(component::VisualMesh{
            .mMesh = aContext.mResources.getShape("models/avocado/Avocado.gltf"),
        })
        .add(component::Text{
            .mString{playerText.str()},
            .mFont = std::move(font),
            .mColor = playerSlot.mColor,
        })
        .add(component::PoseScreenSpace{.mPosition_u =
                {-0.9f + 0.2f * playerSlot.mIndex, 0.8f}})
        .add(component::Collision{component::gPlayerHitbox});
}

bool findSlotAndBind(GameContext & aContext,
                     ent::Phase & aBindPhase,
                     ent::Query<component::PlayerSlot> & aSlots,
                     ControllerType aType,
                     int aIndex)
{
    std::optional<ent::Handle<ent::Entity>> freeSlot = snac::getFirstHandle(
        aSlots,
        [](component::PlayerSlot & aSlot) { return !aSlot.mFilled; });

    if (freeSlot)
    {
        fillSlotWithPlayer(
            aContext, aBindPhase, aType, *freeSlot, aIndex
            );
        return true;
    }

    return false;
}

ent::Handle<ent::Entity>
createMenuItem(GameContext & aContext,
               ent::Phase & aInit,
               std::string aString,
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
         std::string aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale)
{
        auto handle = aContext.mWorld.addEntity();

    handle.get(aPhase)
        ->add(component::Text{
            .mString{std::move(aString)},
            .mFont = std::move(aFont),
            .mColor = aColor,
        })
        .add(component::PoseScreenSpace{.mPosition_u = aPosition_unitscreen,
                                        .mScale = aScale});

        return handle;
}

} // namespace snacgame
} // namespace ad
