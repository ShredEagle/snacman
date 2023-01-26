#include "Entities.h"

#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/Level.h"
#include "component/PlayerLifeCycle.h"
#include "component/PlayerMoveState.h"
#include "component/PoseScreenSpace.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Text.h"

#include <snac-renderer/text/Text.h>

#include <math/Color.h>

namespace ad {
namespace snacgame {

void createLevel(ent::Handle<ent::Entity> & aLevelHandle,
                 ent::EntityManager & aWorld,
                 ent::Phase & aInit,
                 const markovjunior::Grid & aGrid)
{
    aLevelHandle.get(aInit)->add(component::Level(aWorld, aInit, aGrid));
}

void createPill(ent::EntityManager & mWorld,
                ent::Phase & aInit,
                const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)
        ->add(component::Geometry{
            .mSubGridPosition = math::Position<2, float>::Zero(),
            .mGridPosition = aGridPos,
            .mScaling = math::Size<3, float>{.3f, .3f, .3f},
            .mLayer = component::GeometryLayer::Player,
            .mOrientation = math::Quaternion<float>{math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                                                    math::Degree<float>{15.f}},
            .mColor = math::hdr::Rgb<float>{1.f, 0.5f, 0.5f} 
        })
        .add(component::Speed{
            .mRotation = AxisAngle{
                .mAxis = math::UnitVec<3, float>{{1.f, 1.f, 1.f}},
                .mAngle = math::Degree<float>{60.f}
            },
        })
        ;
}

ent::Handle<ent::Entity>
createPathEntity(ent::EntityManager & mWorld,
                 ent::Phase & aInit,
                 const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mColor = math::hdr::gWhite<float>});
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mColor = math::hdr::gRed<float>});
    return handle;
}

ent::Handle<ent::Entity>
createCopPenEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aGridPos)
{
    auto handle = mWorld.addEntity();
    handle.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
        .mColor = math::hdr::gYellow<float>});
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(ent::EntityManager & mWorld,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aGridPos)
{
    auto spawner = mWorld.addEntity();
    spawner.get(aInit)->add(component::Geometry{
        .mSubGridPosition = math::Position<2, float>::Zero(),
        .mGridPosition = aGridPos,
        .mLayer = component::GeometryLayer::Level,
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
        .mColor = math::hdr::gMagenta<float>,
        .mShouldBeDrawn = false});
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

ent::Handle<ent::Entity> makeText(ent::EntityManager & mWorld,
                                  ent::Phase & aPhase,
                                  std::string aString,
                                  std::shared_ptr<snac::Font> aFont,
                                  math::hdr::Rgba_f aColor,
                                  math::Position<2, float> aPosition_unitscreen)
{
    auto handle = mWorld.addEntity();

    handle.get(aPhase)
        ->add(component::Text{
            .mString{std::move(aString)},
            .mFont = std::move(aFont),
            .mColor = aColor,
        })
        .add(component::PoseScreenSpace{.mPosition_u = aPosition_unitscreen})
    ;

    return handle;
}


} // namespace snacgame
} // namespace ad