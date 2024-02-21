#pragma once

#include "Serialization.h"
#include "Reflexion.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <utility>

namespace reflexion {

using json = nlohmann::json;

template<class T_type>
void TypedConstructor<T_type>::construct(ad::ent::Handle<ad::ent::Entity> aHandle, const json & aData)
{
    {
        ad::ent::Phase phase;
        T_type t;
        aHandle.get(phase)->add(t);
    }
    aData & aHandle.get()->get<T_type>();
}

template<class T_type>
void TypedConstructor<T_type>::serialize(json & aData, ad::ent::Handle<ad::ent::Entity> aHandle)
{
    T_type & comp = aHandle.get()->get<T_type>();
    json object = json::object();
    object & comp;
    aData[mName] = object;
}
} // namespace ad
