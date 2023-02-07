#pragma once


#include "RenderThread.h"
// Note: this is coupling the Resource class to the specifics of snacgame. 
// But I do not want to template Resource on the renderer...
#include "simulations/snacgame/Renderer.h"

#include <snac-renderer/Mesh.h>

#include <resource/ResourceManager.h>


namespace ad {
namespace snac {


class Resources
{
public:
    Resources(const resource::ResourceFinder & aFinder, RenderThread<snacgame::Renderer> & aRenderThread) :
        mFinder{aFinder},
        mRenderThread{aRenderThread}
    {}

    std::shared_ptr<Mesh> getShape();

public:
    RenderThread<snacgame::Renderer> & mRenderThread;
    const resource::ResourceFinder & mFinder;
    std::shared_ptr<Mesh> mCube = nullptr;
};


} // namespace snac
} // namespace ad
