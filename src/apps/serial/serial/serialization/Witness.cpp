#include "Witness.h"

#include "../reflexion/Reflexion.h"
#include "../reflexion/Reflexion_impl.h"

#include <entity/Entity.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

void testify_json(ad::ent::EntityManager & aWorld, const Witness && aData)
{
    std::unordered_map<std::string, ad::ent::Handle<ad::ent::Entity>>
        nameHandleMap;
    json * data = std::get<json *>(aData.mData);

    for (auto & [name, ent] : data->items())
    {
        ad::ent::Handle<ad::ent::Entity> handle =
            aWorld.addEntity(name.c_str());
        testify_json(handle, Witness::make_const(&ent));
        nameHandleMap.insert_or_assign(name, handle);
    }

    for (auto & pair : reflexion::handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;
        *handle = nameHandleMap.at(nameRequested);
    }
    reflexion::handleRequestsInstance().clear();
}

void testify_json(ad::ent::Handle<ad::ent::Entity> & aHandle,
                   const Witness && aData)
{
    json * data = std::get<json *>(aData.mData);
    json & components = (*data)["components"];

    for (auto & [name, comp] : components.items())
    {
        std::type_index index =
            reflexion::nameTypeIndexStore().at(name.c_str());
        reflexion::typedProcessorStore().at(index)->addComponentToHandle(
            Witness::make_const(&comp), aHandle);
    }
}

void witness_json(ad::ent::Handle<ad::ent::Entity> & aHandle, Witness && aData)
{
    json * data = std::get<json *>(aData.mData);
    json & componentObject = (*data)["components"];

    for (std::type_index type : aHandle.getTypeSet())
    {
        const char * name = reflexion::typedProcessorStore().at(type)->name();
        json * component = &componentObject[name];
        reflexion::typedProcessorStore().at(type)->serializeComponent(
            Witness::make(component), aHandle);
    }
}

void witness_json(ad::ent::EntityManager & aWorld, Witness && aData)
{
    std::unordered_map<ad::ent::Handle<ad::ent::Entity>, std::string>
        handleNameMap;
    json * data = std::get<json *>(aData.mData);

    aWorld.forEachHandle(
        [data](ad::ent::Handle<ad::ent::Entity> aHandle, const char * aName) {
            json * handle = &(*data)[aName];
            witness_json(aHandle, Witness::make(handle));
        });
}

void witness_imgui(ad::ent::Handle<ad::ent::Entity> & aHandle, Witness && aData)
{
    for (std::type_index type : aHandle.getTypeSet())
    {
        const char * name = reflexion::typedProcessorStore().at(type)->name();
        if (ImGui::TreeNode(name))
        {
            reflexion::typedProcessorStore().at(type)->serializeComponent(
                Witness::make(aData.mData), aHandle);
            ImGui::TreePop();
        }
    }
}

void witness_imgui(ad::ent::EntityManager & aWorld, Witness && aData)
{
    aWorld.forEachHandle(
        [&aData](ad::ent::Handle<ad::ent::Entity> aHandle, const char * aName) {
            if (ImGui::TreeNode(aName))
            {
                witness_imgui(aHandle, Witness::make(aData.mData));
                ImGui::TreePop();
            }
        });
}
