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
    Resources(resource::ResourceFinder aFinder, RenderThread<snacgame::Renderer> & aRenderThread) :
        mFinder{std::move(aFinder)},
        mRenderThread{aRenderThread}
    {}

    std::shared_ptr<Mesh> getShape();

    auto find(const filesystem::path & aPath) const
    { return mFinder.find(aPath); }

public:
    RenderThread<snacgame::Renderer> & mRenderThread;
    resource::ResourceFinder mFinder;
    std::shared_ptr<Mesh> mCube = nullptr;
};


} // namespace snac
} // namespace ad
