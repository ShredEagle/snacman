#pragma once

#include "component/Geometry.h"

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
ent::Handle<ent::Entity> insertTransformNode(ent::EntityManager & aWorld,
                                             component::Geometry aGeometry,
                                             ent::Handle<ent::Entity> aParent);
void insertEntityInScene(ent::Handle<ent::Entity> aHandle,
                         ent::Handle<ent::Entity> aParent);
void removeEntityFromScene(ent::Handle<ent::Entity> aHandle);
} // namespace snacgame
} // namespace ad
