#pragma once

#include "comp.h"
#include "SnacArchive.h"

#include <concepts>
#include <entity/Entity.h>
#include <math/Box.h>
#include <math/Vector.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <type_traits>
#include <utility>

using json = nlohmann::json;

template <class T_type>
using serialNvp = std::pair<const char *, T_type *>;

#define SERIAL_PARAM(instance, name)                                           \
    serialNvp<decltype(instance.name)> { #name, &instance.name }
#define SERIAL_FN_PARAM(instance, name)                                        \
    serialNvp<decltype(instance.name())> { #name, &instance.name() }

/// x: {
///   y: 2,
/// }

template <class T_archive>
concept SnacSerial =
    std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveOut>
    || std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveIn>
    || std::is_same_v<std::remove_cv_t<T_archive>,
                      ad::ent::Handle<ad::ent::Entity>>
    || std::is_same_v<std::remove_cv_t<T_archive>, nlohmann::json>;

template <class T_jsonable>
concept Jsonable = requires(T_jsonable v, json a) {
    {
        a = v
    };
};

template <Jsonable T_data>
json & operator&(json & archive, serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive[name] = *value;
    return archive;
}

template <class T_data>
    requires(!Jsonable<T_data>)
json & operator&(json & archive, serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive[name] & *value;
    return archive;
}

template <Jsonable T_data>
const json & operator&(const json & archive, serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    *value = archive[name];
    return archive;
}

template <class T_data>
    requires(!Jsonable<T_data>)
const json & operator&(const json & archive, serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive[name] & *value;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::CompA & aData)
{
    archive & SERIAL_PARAM(aData, a);
    archive & SERIAL_PARAM(aData, b);
    return archive;
}

void
from_json(ad::ent::Handle<ad::ent::Entity> & aHandle, const json & aData);

void
to_json(ad::ent::Handle<ad::ent::Entity> & aHandle, json & aData);
