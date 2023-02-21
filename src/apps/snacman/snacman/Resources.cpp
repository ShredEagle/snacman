#include "Resources.h"


#include <snac-renderer/ResourceLoad.h>


namespace ad {
namespace snac {


std::shared_ptr<Mesh> Resources::getShape()
{
    if(!mCube)
    {
        mCube = mRenderThread.loadShape(*this)
            .get(); // synchronize call
    }
    return mCube;
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


} // namespace snac
} // namespace ad
