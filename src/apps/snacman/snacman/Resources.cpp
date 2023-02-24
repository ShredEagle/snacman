#include "Resources.h"


#include <snac-renderer/ResourceLoad.h>


namespace ad {
namespace snac {


std::shared_ptr<Mesh> Resources::getShape(filesystem::path aShape)
{
    // This is bad design, but lazy to get the result quickly
    if(aShape.string() == "CUBE")
    {
        if (!mCube)
        {
            mCube = mRenderThread.loadShape(aShape, *this)
                .get(); // synchronize call
        }
        return mCube;
    }
    else
    {
        return mMeshes.load(aShape, mFinder, mRenderThread, *this);
    }
}


std::shared_ptr<Font> Resources::getFont(filesystem::path aFont, unsigned int aPixelHeight)
{
    return mFonts.load(aFont, mFinder, aPixelHeight, mRenderThread, *this);
}


std::shared_ptr<Effect> Resources::getShaderEffect(filesystem::path aProgram)
{
    return mEffects.load(aProgram, mFinder, mRenderThread, *this);
}


std::shared_ptr<Effect> Resources::EffectLoader(
    filesystem::path aProgram, 
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & /*aResources*/)
{
    return loadEffect(aProgram);
}


std::shared_ptr<Font> Resources::FontLoader(
    filesystem::path aFont, 
    unsigned int aPixelHeight,
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadFont(aResources.getFreetype().load(aFont), aPixelHeight, aResources)
        .get(); // synchronize call
}


std::shared_ptr<Mesh> Resources::MeshLoader(
    filesystem::path aMesh, 
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadShape(aMesh, aResources)
            .get(); // synchronize call
}


void Resources::recompilePrograms()
{
    for(auto & [_stringId, effect] : mEffects) 
    {
        *effect = std::move(*loadEffect(effect->mEffectFile));
    }
}


} // namespace snac
} // namespace ad
