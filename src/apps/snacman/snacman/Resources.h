#pragma once


#include "RenderThread.h"
// Note: this is coupling the Resource class to the specifics of snacgame. 
// But I do not want to template Resource on the renderer...
#include "simulations/snacgame/Renderer.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/text/Text.h>

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

    std::shared_ptr<Font> getFont(filesystem::path aFont, unsigned int aPixelHeight);

    auto find(const filesystem::path & aPath) const
    { return mFinder.find(aPath); }

public:
    static std::shared_ptr<Font> FontLoader(
        filesystem::path aFont, 
        unsigned int aPixelHeight,
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);
    
    // There is a smelly circular dependency in this design:
    // Resources knows the RenderThread to request OpenGL related resource loading
    // and the RenderThread uses Resources to load dependent resources (e.g. loading the texture from a model)
    RenderThread<snacgame::Renderer> & mRenderThread;
    resource::ResourceFinder mFinder;

    std::shared_ptr<Mesh> mCube = nullptr;
    resource::ResourceManager<std::shared_ptr<Font>, resource::ResourceFinder, &Resources::FontLoader> mFonts;
};


} // namespace snac
} // namespace ad
