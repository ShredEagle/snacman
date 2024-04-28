#include "Serialization.h"
#include "entity/Entity.h"
#include "handy/StringId.h"
#include "Reflexion.h"
#include "Reflexion_impl.h"

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

void from_json(ad::ent::EntityManager & aWorld, const json & aData)
{
    std::unordered_map<std::string, ad::ent::Handle<ad::ent::Entity>>
        nameHandleMap;

    for (auto & [name, ent] : aData.items())
    {
        ad::ent::Handle<ad::ent::Entity> handle =
            aWorld.addEntity(name.c_str());
        from_json(handle, ent);
        nameHandleMap.insert_or_assign(name, handle);
    }

    for (auto & pair : handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;
        *handle = nameHandleMap.at(nameRequested);
    }
    handleRequestsInstance().clear();
}

void from_json(ad::ent::Handle<ad::ent::Entity> & aHandle, const json & aData)
{
    json components = aData["components"];

    for (auto & [name, comp] : components.items())
    {
        std::type_index index =
            reflexion::nameTypeIndexInstance().at(name.c_str());
        reflexion::indexedTypedConstrutorInstance().at(index)->construct(
            aHandle, comp);
    }
}

void to_json(ad::ent::EntityManager & aWorld, json & aData)
{
    std::unordered_map<ad::ent::Handle<ad::ent::Entity>, std::string> handleNameMap;

    aWorld.forEachHandle([&aData](ad::ent::Handle<ad::ent::Entity> aHandle, const char * aName){
        to_json(aHandle, aData[aName]);
    });
}

void to_json(ad::ent::Handle<ad::ent::Entity> & aHandle, json & aData)
{
    json & components = aData["components"];

    for (std::type_index type : aHandle.getTypeSet())
    {
        reflexion::indexedTypedConstrutorInstance().at(type)->serialize(
            components, aHandle);
    }
}
