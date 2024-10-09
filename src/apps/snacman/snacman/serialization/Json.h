#pragma once

#include <entity/Entity.h>
#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/json.hpp>
#include <reflexion/Concept.h>
#include <reflexion/NameValuePair.h>

template <class T_json_extractable>
concept JsonDefaultExtractable = requires(T_json_extractable v, nlohmann::json a) {
    v = a.template get<T_json_extractable>();
};

template <class T_json_serializable>
concept JsonDefaultSerializable =
    requires(T_json_serializable v, nlohmann::json a) { a = v; };
