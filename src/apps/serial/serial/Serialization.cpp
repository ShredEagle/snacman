#include "handy/StringId.h"
#include "Reflexion.h"
#include "Reflexion_impl.h"

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

void from_json(ad::ent::EntityManager & aWorld, const json & aData)
{
    std::unordered_map<std::string,
                          ad::ent::Handle<ad::ent::Entity>> nameHandleMap;

    if (aData.type() == json::value_t::array)
    {
        return;
    }

    for (auto & ent : aData)
    {
        std::string name =ent["name"].get<std::string>();
        ad::ent::Handle<ad::ent::Entity> handle = aWorld.addEntity(name.c_str());
        from_json(handle, ent);
        nameHandleMap.insert_or_assign(name, handle);
    }

    for (auto & pair : reflexion::handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;
        *handle = nameHandleMap.at(nameRequested);
    }
}

void from_json(ad::ent::Handle<ad::ent::Entity> & aHandle,
                                     const json & aData)
{
    json components = aData["components"];

    for (auto & comp : components)
    {
        std::string compName = comp["name"].get<std::string>();
        std::type_index index = reflexion::nameTypeIndexInstance().at(compName.c_str());
        reflexion::indexedTypedConstrutorInstance().at(index)->construct(aHandle, comp);
    }
}

using json = nlohmann::json;
void to_json(ad::ent::Handle<ad::ent::Entity> & aHandle,
                                     json & aData)
{
    json & components = aData["components"];

    for (std::type_index type : aHandle.getComponentsInfo())
    {
        reflexion::indexedTypedConstrutorInstance().at(type)->serialize(components, aHandle);
    }
}
