#pragma once

#include "component/Geometry.h"
#include "component/InputDeviceDirectory.h"

#include <markovjunior/Grid.h>
#include <entity/Entity.h>
#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {

void createLevel(ent::Handle<ent::Entity> & aLevelHandle, ent::EntityManager & aWorld,
                ent::Phase & aInit,
                const markovjunior::Grid & aGrid);
void createPill(ent::EntityManager & aWorld,
        ent::Phase & aInit,
        const math::Position<2, int> & Pos);
ent::Handle<ent::Entity> createPathEntity(ent::EntityManager & mWorld,
                      ent::Phase & aInit,
                      const math::Position<2, int> & aPos);
ent::Handle<ent::Entity> createPortalEntity(ent::EntityManager & mWorld,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aPos);
ent::Handle<ent::Entity> createCopPenEntity(ent::EntityManager & mWorld,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aPos);
ent::Handle<ent::Entity> createPlayerSpawnEntity(ent::EntityManager & mWorld,
                             ent::Phase & aInit,
                             const math::Position<2, int> & aPos);

ent::Handle<ent::Entity>
createPlayerEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   component::InputDeviceDirectory & aDeviceDirectory,
                   component::ControllerType aControllerType,
                   int aControllerId = 0);

} // namespace snacgame
} // namespace ad
