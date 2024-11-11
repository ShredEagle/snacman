#include "snacman/Logging.h"
#include <fstream>
#include <sstream>
#define GUARD_SERIAL_IMPORT
#include "Witness.h"

#include <cstdio>
#include <entity/Entity.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <snacman/simulations/snacgame/GameContext.h>
#include <snacman/detail/imgui_stdlib.h>
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

void Witness::witness_json(const char * aName,
                           std::shared_ptr<snac::Font> * aObject)
{
    json & data = *std::get<json *>(mData);
    snac::FontSerialData fontData =
        mGameContext.mResources.getFontDataFromResource(*aObject);
    data[aName]["fontPath"] = fontData.mFontPath;
    data[aName]["pixelHeight"] = fontData.mPixelHeight;
    data[aName]["effectPath"] = fontData.mEffectPath;
}

void Witness::testify_json(
    const char * aName,
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

std::vector<ent::Handle<ent::Entity>> testify_json(ent::EntityManager & aWorld, const Witness && aData)
{
    std::unordered_map<std::string, ent::Handle<ent::Entity>> nameHandleMap;
    json * data = std::get<json *>(aData.mData);
    std::vector<ent::Handle<ent::Entity>> resultEntities(data->size());

    int dataIndex = 0;
    for (auto & [name, ent] : data->items())
    {
        ent::Handle<ent::Entity> handle = aWorld.addEntity(name.c_str());
        testify_json(handle, Witness::make_const(&ent, aData.mGameContext));
        resultEntities.at(dataIndex++) = (handle);
    }

    resolveRequestsInstance(aData.mGameContext);

    return resultEntities;
}

void resolveRequestsInstance(snacgame::GameContext & aGameContext)
{

    for (auto & pair : reflexion::handleRequestsInstance())
    {
        auto [handle, nameRequested] = pair;

        ent::Handle<ent::Entity> requestedHandle = aGameContext.mWorld.handleFromName(nameRequested);
        if (requestedHandle.isValid())
        {
            *handle = requestedHandle;
        }
#ifndef NDEBUG
        else
        {
            SELOG(error)("Requested Handle {} does not exists.", nameRequested);
        }
#endif
    }
    reflexion::handleRequestsInstance().clear();
    
}

void testify_json(ent::Handle<ent::Entity> & aHandle, const Witness && aData)
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

    serial::resolveRequestsInstance(aData.mGameContext);
}

void witness_json(ent::Handle<ent::Entity> & aHandle, Witness && aData)
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

void witness_json(ent::EntityManager & aWorld, Witness && aData)
{
    std::unordered_map<ent::Handle<ent::Entity>, std::string> handleNameMap;
    json * data = std::get<json *>(aData.mData);

    aWorld.forEachHandle(
        [data, &aData](ent::Handle<ent::Entity> aHandle, const char * aName) {
            json * handle = &(*data)[aName];
            witness_json(aHandle, Witness::make(handle, aData.mGameContext));
        });
}

void witness_imgui(ent::Handle<ent::Entity> & aHandle, Witness && aData)
{
    bool canBeJsonified = true;
    for (std::type_index type : aHandle.getTypeSet())
    {
        if (reflexion::typedProcessorStore().contains(type))
        {
            const char * name = reflexion::typedProcessorStore().at(type)->name();
            if (ImGui::TreeNode(name))
            {
                    reflexion::typedProcessorStore().at(type)->serializeComponent(
                        Witness::make(aData.mData, aData.mGameContext), aHandle);
                ImGui::TreePop();
            }
        }
        else
        {
            canBeJsonified = false;
            ImGui::Text("Can't show this component");
        }
    }
    static std::string blueprint;
    static bool showBlueprint;
    static int lineCount = 0;
    if (canBeJsonified && ImGui::Button("Get blueprint"))
    {
        json jsonSerial;
        witness_json(aHandle, Witness::make(&jsonSerial, aData.mGameContext));
        blueprint = jsonSerial.dump(2);
        showBlueprint = true;
        lineCount = (int)std::count(blueprint.begin(), blueprint.end(), '\n');
    }
    if (showBlueprint)
    {
        ImGui::InputTextMultiline("blueprint data", &blueprint, ImVec2(0, ImGui::GetTextLineHeight() * (float)(lineCount + 2)));
    }
}

void witness_imgui(ent::EntityManager & aWorld, Witness && aData)
{
    aWorld.forEachHandle(
        [&aData](ent::Handle<ent::Entity> aHandle, const char * aName) {
            if (ImGui::TreeNode(aName))
            {
                witness_imgui(aHandle,
                              Witness::make(aData.mData, aData.mGameContext));
                ImGui::TreePop();
            }
        });
}

void Witness::witness_imgui(const char * aName,
                           renderer::Handle<const renderer::Object> * aObject)
{
    snac::ModelData modelData =
        mGameContext.mResources.getModelDataFromResource(*aObject);
    ImGui::Text("model path: %s", modelData.mModelPath.c_str());
    ImGui::Text("effect path: %s", modelData.mEffectPath.c_str());
}

void Witness::witness_imgui(const char * aName,
                           std::shared_ptr<snac::Font> * aObject)
{
    snac::FontSerialData fontData =
        mGameContext.mResources.getFontDataFromResource(*aObject);
    ImGui::Text("path: %s", fontData.mFontPath.c_str());
    ImGui::Text("pixel height: %d", fontData.mPixelHeight);
    ImGui::Text("effect path: %s", fontData.mEffectPath.c_str());
}

EntityLedger loadLedgerFromJson(const filesystem::path & aJsonPath,
                          ent::EntityManager & aWorld,
                          snacgame::GameContext & aContext)
{
    filesystem::path realJsonPath = *(aContext.mResources.find(aJsonPath));
    std::ifstream sourceStream(realJsonPath);
    json sourceJson = json::parse(sourceStream);
    std::vector<ent::Handle<ent::Entity>> entities =
        testify_json(aWorld, Witness::make_const(&sourceJson, aContext));
    EntityLedger resultLedger{
        .mSourcePath = aJsonPath,
        .mEntities = std::move(entities),
        .mJson = sourceJson,
        .mWorld = aWorld,
    };
    return resultLedger;
}

} // namespace serial
} // namespace ad
