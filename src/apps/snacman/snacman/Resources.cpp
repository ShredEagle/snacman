#include "Resources.h"

#include "RenderThread.h"                   // for RenderThread

#include <resource/ResourceManager.h>               // for ResourceManager
                                                    //
#include <algorithm>                                // for copy, max
#include <future>                                   // for future
#include <string>                                   // for operator==, string

namespace ad {
namespace snac {

struct Effect;
struct Font;
struct Model;

Resources::~Resources()
{
    SELOG(debug)("Destructing snacman::Resources instance.");
}


snacgame::Handle<renderer::Node> Resources::getModel(filesystem::path aModel, filesystem::path aEffect)
{
    // This is bad design, but lazy to get the result quickly
    if(aModel.string() == "CUBE")
    {
        throw std::invalid_argument("'CUBE' hardcoded model is now deprecated.");
    }
    else
    {
        //return mModels.load(aModel, mFinder, aEffect, mRenderThread, mResources_V2);
        // TODO Ad 2024/02/14: #RV2 Restore actual models name
        return mModels.load("models/donut/donut.seum", mFinder, aEffect, mRenderThread, mResources_V2);
    }
}


std::shared_ptr<Font> Resources::getFont(filesystem::path aFont,
                                         unsigned int aPixelHeight,
                                         filesystem::path aEffect)
{
    return mFonts.load(aFont, mFinder, aPixelHeight, aEffect, mRenderThread, *this);
}


std::shared_ptr<Effect> Resources::getShaderEffect(filesystem::path aEffect)
{
    return mEffects.load(aEffect, mFinder, mRenderThread, *this);
}


std::shared_ptr<Effect> Resources::EffectLoader(
    filesystem::path aEffect, 
    RenderThread<snacgame::Renderer_t> & aRenderThread,
    Resources & aResources)
{
    return loadEffect(aEffect, aResources.getTechniqueLoader());
}


std::shared_ptr<Font> Resources::FontLoader(
    filesystem::path aFont, 
    unsigned int aPixelHeight,
    filesystem::path aEffect,
    RenderThread<snacgame::Renderer_t> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadFont(aResources.getFreetype().load(aFont), aPixelHeight, aEffect, aResources)
        .get(); // synchronize call
}


snacgame::Handle<renderer::Node> Resources::ModelLoader(
    filesystem::path aModel, 
    filesystem::path aEffect,
    RenderThread<snacgame::Renderer_t> & aRenderThread,
    snacgame::Resources_V2 & aResources)
{
    return aRenderThread.loadModel(aModel, aEffect, aResources)
            .get(); // synchronize call
}


void Resources::recompilePrograms()
{
    // Note: this is a naive approach, I suspect this could lead to data races
    bool allSuccess = true;
    for(auto & [_stringId, effect] : mEffects) 
    {
        for (auto & technique : effect->mTechniques)
        {
            if (!attemptRecompile(technique))
            {
                allSuccess = false;
            }
        }
    }

    if(allSuccess)
    {
        SELOG(info)("All programs were recompiled successfully.");
    }
}


} // namespace snac
} // namespace ad
