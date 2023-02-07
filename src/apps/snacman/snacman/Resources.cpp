#include "Resources.h"


namespace ad {
namespace snac {


std::shared_ptr<Mesh> Resources::getShape()
{
    if(!mCube)
    {
        mCube = mRenderThread.loadShape(mFinder).get();
    }
    return mCube;
}


} // namespace snac
} // namespace ad
