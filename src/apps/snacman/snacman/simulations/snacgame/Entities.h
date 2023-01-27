#pragma once

#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/PlayerSlot.h"

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

void 
fillSlotWithPlayer(ent::Phase & aInit,
                   component::ControllerType aControllerType,
                   ent::Handle<ent::Entity> aSlot,
                   int aControllerId = 0);

ent::Handle<ent::Entity> createMenuItem(ent::EntityManager & aWorld,
                                        ent::Phase & aInit,
                                        const math::Position<2, int> & aPos);

} // namespace snacgame
} // namespace ad
