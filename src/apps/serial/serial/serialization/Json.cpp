#include "Json.h"
#include "../reflexion/Reflexion.h"
#include "../reflexion/Reflexion_impl.h"

#include "handy/StringId.h"
#include "entity/Entity.h"
#include "serial/serialization/JsonWitness.h"

#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

void from_json(ad::ent::EntityManager & aWorld, const JsonWitness && aData)
{
    std::unordered_map<std::string, ad::ent::Handle<ad::ent::Entity>>
        nameHandleMap;

    for (auto & [name, ent] : aData.mData.items())
    {
        ad::ent::Handle<ad::ent::Entity> handle =
            aWorld.addEntity(name.c_str());
        from_json(handle, JsonWitness::make_const(ent));
        nameHandleMap.insert_or_assign(name, handle);
    }

    for (auto & pair : handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;
        *handle = nameHandleMap.at(nameRequested);
    }
    handleRequestsInstance().clear();
}

void from_json(ad::ent::Handle<ad::ent::Entity> & aHandle, const JsonWitness && aData)
{
    json & components = aData.mData["components"];

    for (auto & [name, comp] : components.items())
    {
        std::type_index index =
            reflexion::nameTypeIndexStore().at(name.c_str());
        reflexion::typedProcessorStore().at(index)->addComponentToHandle(
            aHandle, JsonWitness::make_const(comp));
    }
}

void to_json(ad::ent::EntityManager & aWorld, JsonWitness && aData)
{
    std::unordered_map<ad::ent::Handle<ad::ent::Entity>, std::string> handleNameMap;

    aWorld.forEachHandle([&aData](ad::ent::Handle<ad::ent::Entity> aHandle, const char * aName){
        to_json(aHandle, JsonWitness::make(aData.mData[aName]));
    });
}

void to_json(ad::ent::Handle<ad::ent::Entity> & aHandle, JsonWitness && aData)
{
    json & componentJson = aData.mData["components"];

    for (std::type_index type : aHandle.getTypeSet())
    {
        reflexion::typedProcessorStore().at(type)->serializeComponent(
            JsonWitness::make(componentJson), aHandle);
    }
}
