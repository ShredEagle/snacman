#include "Resources.h"

#include "RenderThread.h"                   // for RenderThread
#include "simulations/snacgame/Renderer.h"  // for Renderer

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


std::shared_ptr<Model> Resources::getModel(filesystem::path aModel)
{
    // This is bad design, but lazy to get the result quickly
    if(aModel.string() == "CUBE")
    {
        if (!mCube)
        {
            mCube = mRenderThread.loadModel(aModel, *this)
                .get(); // synchronize call
        }
        return mCube;
    }
    else
    {
        return mModels.load(aModel, mFinder, mRenderThread, *this);
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
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return loadEffect(aEffect, aResources.getTechniqueLoader());
}


std::shared_ptr<Font> Resources::FontLoader(
    filesystem::path aFont, 
    unsigned int aPixelHeight,
    filesystem::path aEffect,
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadFont(aResources.getFreetype().load(aFont), aPixelHeight, aEffect, aResources)
        .get(); // synchronize call
}


std::shared_ptr<Model> Resources::ModelLoader(
    filesystem::path aModel, 
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadModel(aModel, aResources)
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
