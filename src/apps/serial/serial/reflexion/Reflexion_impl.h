#pragma once

#include "Reflexion.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <nlohmann/json.hpp>

namespace reflexion {

using json = nlohmann::json;

template<class T_type>
void TypedProcessor<T_type>::addComponentToHandle(ad::ent::Handle<ad::ent::Entity> aHandle, const JsonWitness && aWitness)
{
    {
        ad::ent::Phase phase;
        T_type t;
        aHandle.get(phase)->add(t);
    }
    aHandle.get()->get<T_type>().describeTo(aWitness);
}

template<class T_type>
void TypedProcessor<T_type>::serializeComponent(JsonWitness && aWitness, ad::ent::Handle<ad::ent::Entity> aHandle)
{
    T_type & comp = aHandle.get()->get<T_type>();
    comp.describeTo(JsonWitness::make(aWitness.mData[mName]));
}
} // namespace ad
