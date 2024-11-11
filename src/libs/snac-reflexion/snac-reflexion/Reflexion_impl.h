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
    ad::ent::Phase phase;
    if constexpr (std::is_default_constructible_v<T_type>)
    {
        T_type t;
        t.describeTo(aWitness);
        aHandle.get(phase)->add(t);
    }
    else
    {
        aHandle.get(phase)->add(T_type::construct(aWitness));
    }
}

template<class T_type>
void TypedProcessor<T_type>::serializeComponent(ad::serial::Witness && aWitness, ad::ent::Handle<ad::ent::Entity> aHandle)
{
    T_type & comp = aHandle.get()->get<T_type>();
    comp.describeTo(aWitness);
}
} // namespace ad
