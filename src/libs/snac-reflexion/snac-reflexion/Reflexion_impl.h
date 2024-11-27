#pragma once

#include "Reflexion.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <nlohmann/json.hpp>
#include <type_traits>

namespace reflexion {

using json = nlohmann::json;

template<class T_type>
void TypedProcessor<T_type>::addComponentToHandle(const ad::serial::Witness && aWitness, ad::ent::Handle<ad::ent::Entity> aHandle)
{
    T_type t;
    {
        ad::ent::Phase phase;
        aHandle.get(phase)->add(t);
    }
    aHandle.get()->get<T_type>().describeTo(aWitness);
}

template<class T_type>
void TypedProcessor<T_type>::serializeComponent(ad::serial::Witness && aWitness, ad::ent::Handle<ad::ent::Entity> aHandle)
{
    T_type & comp = aHandle.get()->get<T_type>();
    comp.describeTo(aWitness);
}
} // namespace ad
