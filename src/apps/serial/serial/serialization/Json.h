#pragma once

#include <entity/Entity.h>
#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/json.hpp>
#include <reflexion/Concept.h>
#include <reflexion/NameValuePair.h>

class JsonWitness;

template <class T_json_extractable>
concept JsonExtractable = requires(T_json_extractable v, nlohmann::json a) {
    v = a.template get<T_json_extractable>();
};

template <class T_json_serializable>
concept JsonSerializable =
    requires(T_json_serializable v, nlohmann::json a) { a = v; };

void from_json(ad::ent::EntityManager & aWorld, const JsonWitness && aData);
void from_json(ad::ent::Handle<ad::ent::Entity> & aHandle,
               const JsonWitness && aData);

void to_json(ad::ent::EntityManager & aWorld, JsonWitness && aData);
void to_json(ad::ent::Handle<ad::ent::Entity> & aHandle, JsonWitness && aData);
