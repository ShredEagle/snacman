#pragma once

#include "SnacArchive.h"
#include "common.h"

#include <concepts>
#include <entity/Entity.h>
#include <istream>
#include <math/Box.h>
#include <math/Vector.h>
#include <nlohmann/json.hpp>
#include <ostream>
#include <type_traits>
#include <utility>

using json = nlohmann::json;

template <class T_archive>
concept SnacSerial =
    std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveOut>
    || std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveIn>
    || std::is_same_v<std::remove_cv_t<T_archive>, nlohmann::json>;

template <class T_jsonable>
concept Jsonable = requires(T_jsonable v, json a) {
    {
        a = v
    };
};

template <class T_Serializable>
concept Serializable = requires(T_Serializable v, json a) {
    {
        v.serialize(a)
    };
};

template <Jsonable T_data>
json & operator&(json & archive, serialNvp<T_data> aData);

template <class T_data>
    requires(!Jsonable<T_data>)
json & operator&(json & archive, serialNvp<T_data> aData);

template <Jsonable T_data>
const json & operator&(const json & archive, serialNvp<T_data> aData);

template <class T_data>
    requires(!Jsonable<T_data>)
const json & operator&(const json & archive, serialNvp<T_data> aData);

template <SnacSerial T_archive, Serializable T_data>
T_archive & operator&(T_archive & archive, T_data & aData);

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

template <Archivable T_data>
SnacArchiveOut & operator&(SnacArchiveOut & archive,
                               serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive & name & " : " & *value & "\n";
    return archive;
}

template <class T_data>
    requires(!Archivable<T_data>)
SnacArchiveOut & operator&(SnacArchiveOut & archive,
                               serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive & name & " {" & "\n";
    archive & *value;
    archive & "}" & "\n";
    return archive;
}

inline std::vector<std::pair<ad::ent::Handle<ad::ent::Entity> *, std::string>> &
handleRequestsInstance()
{
    static std::vector<
        std::pair<ad::ent::Handle<ad::ent::Entity> *, std::string>>
        handleRequestsStore = {};
    return handleRequestsStore;
}

template <>
inline json & operator&(json & archive,
                        serialNvp<ad::ent::Handle<ad::ent::Entity>> aData)
{
    auto & [name, handle] = aData;
    archive[name] = handle->name();
    return archive;
}

template <>
inline const json & operator&(const json & archive,
                              serialNvp<ad::ent::Handle<ad::ent::Entity>> aData)
{
    auto & [name, handle] = aData;
    handleRequestsInstance().emplace_back(handle, archive[name]);
    return archive;
}

template <>
inline SnacArchiveOut & operator&(SnacArchiveOut & archive,
                        serialNvp<ad::ent::Handle<ad::ent::Entity>> aData)
{
    auto & [name, handle] = aData;
    archive & name & " : " & handle->name() & "\n";
    return archive;
}

template <SnacSerial T_archive, Serializable T_data>
T_archive & operator&(T_archive & archive, T_data & aData)
{
    aData.serialize(archive);
    return archive;
}

void from_json(ad::ent::EntityManager & aWorld, const json & aData);
void from_json(ad::ent::Handle<ad::ent::Entity> & aHandle, const json & aData);

void to_json(ad::ent::EntityManager & aWorld, json & aData);
void to_json(ad::ent::Handle<ad::ent::Entity> & aHandle, json & aData);
