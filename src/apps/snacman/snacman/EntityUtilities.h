// This file is an illustration of why Entity interface is too cumbersome
#pragma once


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


} // namespace snac
} // namespace ad
