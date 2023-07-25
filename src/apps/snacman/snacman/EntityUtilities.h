// This file is an illustration of why Entity interface is too cumbersome
#pragma once

#include "entity/EntityManager.h"
#include "handy/Guard.h"

#include <entity/Entity.h>

namespace ad {
namespace snac {

template <class T_component>
T_component & getComponent(ent::Handle<ent::Entity> aHandle)
{
    auto view = aHandle.get();
    assert(view.has_value());
    assert(view->has<T_component>());
    return view->get<T_component>();
}

/* class GuardedEntity */
/* { */
/* public: */
/*     GuardedEntity(ent::EntityManager & aEntityManager) : */
/*         mEntity{aEntityManager.addEntity(), [](ent::Handle<ent::Entity> handle) */
/*                 { */
/*                     ent::Phase destroy; */
/*                     handle.get(destroy)->erase(); */
/*                 }} */
/*     {} */

/*     GuardedEntity(ent::Handle<ent::Entity> aHandle) : */
/*         mEntity{aHandle, [](ent::Handle<ent::Entity> handle) */
/*                 { */
/*                     ent::Phase destroy; */
/*                     handle.get(destroy)->erase(); */
/*                 }} */
/*     {} */

/*     GuardedEntity(const GuardedEntity &) = delete; */
/*     GuardedEntity(GuardedEntity &&) = delete; */
/*     GuardedEntity & operator=(const GuardedEntity &) = delete; */
/*     GuardedEntity & operator=(GuardedEntity &&) = delete; */
/*     ~GuardedEntity() = default; */

/*     operator ent::Handle<ent::Entity>() { return mEntity.get(); } */

/*     ent::Handle<ent::Entity> * operator->() { return &mEntity.get(); } */
/*     ent::Handle<ent::Entity> get() { return mEntity.get(); } */

/* private: */
/*     ResourceGuard<ent::Handle<ent::Entity>> mEntity; */
/* }; */

} // namespace snac
} // namespace ad
