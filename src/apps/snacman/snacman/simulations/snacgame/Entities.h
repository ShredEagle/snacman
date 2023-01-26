#pragma once

#include "component/Geometry.h"
#include "context/InputDeviceDirectory.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <markovjunior/Grid.h>

namespace ad {
namespace snacgame {

void createPill(ent::EntityManager & aWorld,
                ent::Phase & aInit,
                const math::Position<2, int> & Pos);
ent::Handle<ent::Entity> createPathEntity(ent::EntityManager & mWorld,
                                          ent::Phase & aInit,
                                          const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createPortalEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createCopPenEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createPlayerSpawnEntity(ent::EntityManager & mWorld,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aPos);

ent::Handle<ent::Entity>
createPlayerEntity(ent::EntityManager & mWorld,
                   ent::Phase & aInit,
                   InputDeviceDirectory & aDeviceDirectory,
                   component::ControllerType aControllerType,
                   int aControllerId = 0);

ent::Handle<ent::Entity> createMenuItem(ent::EntityManager & aWorld,
                                        ent::Phase & aInit,
                                        const math::Position<2, int> & aPos);

} // namespace snacgame
} // namespace ad
