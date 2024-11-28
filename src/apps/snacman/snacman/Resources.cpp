#include "Resources.h"

#include "RenderThread.h"                  // for RenderThread
#include "simulations/snacgame/Renderer.h" // for Renderer
#include "snacman/serialization/Witness.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <resource/ResourceManager.h> // for ResourceManager
                                      //
#include <algorithm>                  // for copy, max
#include <future>                     // for future
#include <string>                     // for operator==, string

namespace ad::snac {

Resources::~Resources()
{
    SELOG(debug)("Destructing snacman::Resources instance.");
}

snacgame::Renderer_t::Handle_t<const renderer::Object>
Resources::getModel(filesystem::path aModel, filesystem::path aEffect)
{
    // TODO Ad 2024/03/27: Since it will reuse the effect of first load,
    // we should explicitly error if an already-loader model is requested with a
    // different effect file.

    // TODO: remove this cube section when we fill confident there are not
    // requests left in the code This was bad design, but lazy to get the result
    // quickly
    if (aModel.string() == "CUBE")
    {
        throw std::invalid_argument(
            "'CUBE' hardcoded model is now deprecated.");
    }
    else
    {
        if (aModel.extension() == ".gltf")
        {
            aModel.replace_extension(".seum");
            SELOG(warn)("Live patching extension of '{}'.", aModel.string());
        }
        auto model = mModels.load(aModel, mFinder, aEffect, mRenderThread,
                                  mResources_V2);
        // This should be in resource manager however load is a variadic
        // template function It is not possible to declare a store for those
        // parameters inside ResourceManager because those are not template
        // parameters of ResourceManager
        mModelDataFromResource.insert_or_assign(model,
                                                ModelData{aModel, aEffect});
        return model;
    }
}

std::shared_ptr<Resources::LoadedFont_t> Resources::getFont(filesystem::path aFont,
                                                            unsigned int aPixelHeight)
{
    auto font = mFonts.load(aFont, mFinder, aPixelHeight,
                            mRenderThread, *this);
    mFontDataFromResource.insert_or_assign(
        font, FontSerialData{aFont, aPixelHeight});
    return font;
}

ent::Handle<ent::Entity>
Resources::getBlueprint(const filesystem::path & aBpFile,
                        ent::EntityManager & aWorld,
                        snacgame::GameContext & aGameContext)
{
    ent::Handle<ent::Entity> handle =
        mBlueprints.load(aBpFile, mFinder, aWorld, aGameContext);
    return handle;
}

std::shared_ptr<Effect> Resources::getShaderEffect(filesystem::path aEffect)
{
    return mEffects.load(aEffect, mFinder, mRenderThread, *this);
}

std::shared_ptr<Effect>
Resources::EffectLoader(filesystem::path aEffect,
                        RenderThread<snacgame::Renderer_t> & aRenderThread,
                        Resources & aResources)
{
    return loadEffect(aEffect, aResources.getTechniqueLoader());
}

std::shared_ptr<Resources::LoadedFont_t>
Resources::FontLoader(filesystem::path aFont,
                      unsigned int aPixelHeight,
                      RenderThread<snacgame::Renderer_t> & aRenderThread,
                      Resources & aResources)
{
    return aRenderThread
        .loadFont(aResources.getFreetype(),
                  std::move(aFont),
                  aPixelHeight,
                  aResources)
        .get(); // synchronize call
}

snacgame::Renderer_t::Handle_t<const renderer::Object>
Resources::ModelLoader(filesystem::path aModel,
                       filesystem::path aEffect,
                       RenderThread<snacgame::Renderer_t> & aRenderThread,
                       snacgame::Resources_V2 & aResources)
{
    return aRenderThread.loadModel(aModel, aEffect, aResources)
        .get(); // synchronize call
}

using json = nlohmann::json;

// TODO(franz): Reimplement after serialization

ent::Handle<ent::Entity>
Resources::BpLoader(const filesystem::path & aBpFile,
                    ent::EntityManager & aWorld,
                    snacgame::GameContext & aGameContext)
{
    std::ifstream bpStream(aBpFile);
    json data = json::parse(bpStream);
    ent::Handle<ent::Entity> bp =
        aWorld.addBlueprint(data["name"].get<std::string>().c_str());
    serial::testify_json(bp, serial::Witness::make_const(&data["data"], aGameContext));
    return bp;
}

void Resources::recompilePrograms()
{
    // Note: this is a naive approach, I suspect this could lead to data races
    bool allSuccess = true;
    for (auto & [_stringId, effect] : mEffects)
    {
        for (auto & technique : effect->mTechniques)
        {
            if (!attemptRecompile(technique))
            {
                allSuccess = false;
            }
        }
    }

    if (allSuccess)
    {
        SELOG(info)("All programs were recompiled successfully.");
    }
}

} // namespace ad::snac
