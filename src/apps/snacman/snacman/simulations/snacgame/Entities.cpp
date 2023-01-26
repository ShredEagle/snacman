#include "Entities.h"

#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/LevelData.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/Spawner.h"

#include <math/Color.h>

namespace ad {
namespace snacgame {

void createPill(ent::EntityManager & mWorld,
                ent::Phase & aInit,
                const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mScaling = math::Size<3, float>{.3f, .3f, .3f},
        .mLayer = component::GeometryLayer::Player,
        .mYRotation = math::Degree<float>{15.f},
        .mColor = math::hdr::Rgb<float>{1.f, 0.5f, 0.5f}});
}

ent::Handle<ent::Entity>
createPathEntity(ent::EntityManager & mWorld,
                 ent::Phase & aInit,
                 const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gWhite<float>});
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gRed<float>});
    return handle;
}

ent::Handle<ent::Entity>
createCopPenEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::LevelEntity{});
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gYellow<float>});
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(ent::EntityManager & mWorld,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aGridPos)
{
    auto spawner = mWorld.addEntity();
    spawner.get(aInit)->add(component::LevelEntity{});
    spawner.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gCyan<float>});
    spawner.get(aInit)->add(component::Spawner{.mSpawnPosition = aGridPos});

    return spawner;
}

ent::Handle<ent::Entity>
createPlayerEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   InputDeviceDirectory & aDeviceDirectory,
                   component::ControllerType aControllerType,
                   int aControllerId)
{
    auto playerHandle = mWorld.addEntity();
    auto playerEntity = playerHandle.get(aInit);

    playerEntity->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = math::Position<2, int>::Zero(),
        .mLayer = component::GeometryLayer::Player,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gMagenta<float>});
    playerEntity->add(component::PlayerLifeCycle{});

    if (aControllerType == component::ControllerType::Keyboard)
    {
        aDeviceDirectory.bindPlayerToKeyboard(aInit, playerHandle);
    }
    else if (aControllerType == component::ControllerType::Gamepad)
    {
        aDeviceDirectory.bindPlayerToGamepad(aInit, playerHandle, aControllerId);
    }

    playerEntity->add(component::PlayerMoveState{});

    return playerHandle;
}

ent::Handle<ent::Entity>
createMenuItem(ent::EntityManager &aWorld, ent::Phase & aInit, const math::Position<2, int> &aPos)
{
    auto menuItem = aWorld.addEntity();
    menuItem.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aPos,
        .mLayer = component::GeometryLayer::Menu,
        .mYRotation = math::Degree<float>{0.f},
        .mColor = math::hdr::gMagenta<float>});

    return menuItem;
}
} // namespace snacgame
} // namespace ad
