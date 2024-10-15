#include "Witness.h"

#include <snacman/simulations/snacgame/GameContext.h>

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>

#include <entity/Entity.h>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>

namespace ad {
namespace serial {

using nlohmann::json;

void Witness::witness_json(const char * aName,
                  renderer::Handle<const renderer::Object> * aObject)
{
    json & data = *std::get<json *>(mData);
    snac::ModelData modelData =
        mGameContext.mResources.getModelDataFromResource(*aObject);
    data[aName]["modelPath"] = modelData.mModelPath;
    data[aName]["effectPath"] = modelData.mEffectPath;
}

void Witness::witness_json(const char * aName, std::shared_ptr<snac::Font> * aObject)
{
    json & data = *std::get<json *>(mData);
    snac::FontSerialData fontData =
        mGameContext.mResources.getFontDataFromResource(*aObject);
    data[aName]["fontPath"] = fontData.mFontPath;
    data[aName]["pixelHeight"] = fontData.mPixelHeight;
    data[aName]["effectPath"] = fontData.mEffectPath;
}

void Witness::testify_json(const char * aName,
                  renderer::Handle<const renderer::Object> * aObject) const
{
    json & data = *std::get<json *>(mData);
    *aObject = mGameContext.mResources.getModel(data[aName]["modelPath"],
                                                data[aName]["effectPath"]);
}

void Witness::testify_json(const char * aName,
                  std::shared_ptr<snac::Font> * aObject) const
{
    json & data = *std::get<json *>(mData);
    *aObject = mGameContext.mResources.getFont(data[aName]["fontPath"],
                                               data[aName]["pixelHeight"],
                                               data[aName]["effectPath"]);
}

void testify_json(ent::EntityManager & aWorld,
                  const Witness && aData)
{
    std::unordered_map<std::string, ent::Handle<ent::Entity>> nameHandleMap;
    json * data = std::get<json *>(aData.mData);

    for (auto & [name, ent] : data->items())
    {
        ent::Handle<ent::Entity> handle = aWorld.addEntity(name.c_str());
        testify_json(handle, Witness::make_const(&ent, aData.mGameContext));
        nameHandleMap.insert_or_assign(name, handle);
    }

    for (auto & pair : reflexion::handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;
        // TODO(franz): maybe add the invalid handle to the nameHandleMap with a
        // special name
        if (nameHandleMap.contains(nameRequested))
        {
            *handle = nameHandleMap.at(nameRequested);
        }
    }
    reflexion::handleRequestsInstance().clear();
}

void testify_json(ent::Handle<ent::Entity> & aHandle,
                  const Witness && aData)
{
    json * data = std::get<json *>(aData.mData);
    json & components = (*data)["components"];

    for (auto & [name, comp] : components.items())
    {
#ifndef NDEBUG
        assert(reflexion::nameTypeIndexStore().contains(name)
               || !fprintf(stderr, "no constructor for component %s\n",
                           name.c_str()));
#endif // DEBUG
        std::type_index index =
            reflexion::nameTypeIndexStore().at(name.c_str());
        reflexion::typedProcessorStore().at(index)->addComponentToHandle(
            Witness::make_const(&comp, aData.mGameContext), aHandle);
    }
}

void witness_json(ent::Handle<ent::Entity> & aHandle,
                  Witness && aData)
{
    json * data = std::get<json *>(aData.mData);
    json & componentObject = (*data)["components"];

    for (std::type_index type : aHandle.getTypeSet())
    {
#ifndef NDEBUG
        assert(reflexion::typedProcessorStore().contains(type)
               || !fprintf(stderr, "no constructor for %s\n", type.name()));
#endif // DEBUG
        const char * name = reflexion::typedProcessorStore().at(type)->name();
        json * component = &componentObject[name];
        reflexion::typedProcessorStore().at(type)->serializeComponent(
            Witness::make(component, aData.mGameContext), aHandle);
    }
}

void witness_json(ent::EntityManager & aWorld,
                  Witness && aData)
{
    std::unordered_map<ent::Handle<ent::Entity>, std::string> handleNameMap;
    json * data = std::get<json *>(aData.mData);

    aWorld.forEachHandle(
        [data, &aData](ent::Handle<ent::Entity> aHandle, const char * aName) {
            json * handle = &(*data)[aName];
            witness_json(aHandle, Witness::make(handle, aData.mGameContext));
        });
}

void witness_imgui(ent::Handle<ent::Entity> & aHandle,
                   Witness && aData)
{
    for (std::type_index type : aHandle.getTypeSet())
    {
        const char * name = reflexion::typedProcessorStore().at(type)->name();
        if (ImGui::TreeNode(name))
        {
            reflexion::typedProcessorStore().at(type)->serializeComponent(
                Witness::make(aData.mData, aData.mGameContext), aHandle);
            ImGui::TreePop();
        }
    }
}

void witness_imgui(ent::EntityManager & aWorld,
                   Witness && aData)
{
    aWorld.forEachHandle(
        [&aData](ent::Handle<ent::Entity> aHandle, const char * aName) {
            if (ImGui::TreeNode(aName))
            {
                witness_imgui(aHandle, Witness::make(aData.mData, aData.mGameContext));
                ImGui::TreePop();
            }
        });
}

} // namespace serial
} // namespace ad
