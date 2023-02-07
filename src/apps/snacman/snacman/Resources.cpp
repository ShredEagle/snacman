#include "Resources.h"


namespace ad {
namespace snac {


std::shared_ptr<Mesh> Resources::getShape()
{
    if(!mCube)
    {
        mCube = mRenderThread.loadShape(mFinder)
            .get(); // synchronize call
    }
    return mCube;
}

std::shared_ptr<Font> Resources::getFont(filesystem::path aFont, unsigned int aPixelHeight)
{
    return mFonts.load(aFont, mFinder, aPixelHeight, mRenderThread, *this);
}

std::shared_ptr<Font> Resources::FontLoader(
    filesystem::path aFont, 
    unsigned int aPixelHeight,
    RenderThread<snacgame::Renderer> & aRenderThread,
    Resources & aResources)
{
    return aRenderThread.loadFont(aFont, aPixelHeight, aResources)
        .get(); // synchronize call
}


} // namespace snac
} // namespace ad
