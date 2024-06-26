#pragma once

#include "Serialization.h"
#include "snacman/detail/Reflexion.h"

namespace ad {
namespace snacgame {
namespace detail {

template<class T_type>
void TypedConstructor<T_type>::construct(ent::Handle<ent::Entity> aHandle, json & aData)
{
    ent::Phase phase;
    T_type t;
    aData & t;
    aHandle.get(phase)->add(t);
}

} // namespace detail
} // namespace snacgame
} // namespace ad
