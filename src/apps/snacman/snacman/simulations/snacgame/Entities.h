#pragma once

#include "component/Geometry.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {

void createPathEntity(ent::EntityManager &mWorld, ent::Phase &aInit, const math::Position<3, float> & aPos);
void createPortalEntity(ent::EntityManager &mWorld, ent::Phase &aInit, const math::Position<3, float> & aPos);
void createCopPenEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos);
void createPlayerSpawnEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos);

void createPlayerEntity(ent::EntityManager & mWorld, ent::Phase & aInit);

} // snacgame
} // ad
