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
    Resources(resource::ResourceFinder aFinder,
              arte::Freetype & aFreetype,
              RenderThread<snacgame::Renderer> & aRenderThread) :
        mFinder{std::move(aFinder)},
        mFreetype{aFreetype},
        mRenderThread{aRenderThread}
    {}

    // Just to log it
    ~Resources();

    std::shared_ptr<Mesh> getShape(filesystem::path aShape);

    std::shared_ptr<Font> getFont(filesystem::path aFont, unsigned int aPixelHeight);

    /// \warning At the moment: intended to be called only from the thread where OpenGL context is active.
    std::shared_ptr<Effect> getTrivialShaderEffect(filesystem::path aProgram);

    auto find(const filesystem::path & aPath) const
    { return mFinder.find(aPath); }

    const arte::Freetype & getFreetype()
    { return mFreetype; }
    
    void recompilePrograms();

private:
    static std::shared_ptr<Font> FontLoader(
        filesystem::path aFont, 
        unsigned int aPixelHeight,
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);

    static std::shared_ptr<Effect> EffectLoader(
        filesystem::path aProgram, 
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);

    static std::shared_ptr<Mesh> MeshLoader(
        filesystem::path aMesh, 
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);
    
    // There is a smelly circular dependency in this design:
    // Resources knows the RenderThread to request OpenGL related resource loading
    // and the RenderThread uses Resources to load dependent resources (e.g. loading the texture from a model)
    resource::ResourceFinder mFinder;
    arte::Freetype & mFreetype;
    RenderThread<snacgame::Renderer> & mRenderThread;
    
    std::shared_ptr<Mesh> mCube = nullptr;
    resource::ResourceManager<std::shared_ptr<Font>,   resource::ResourceFinder, &Resources::FontLoader>   mFonts;
    resource::ResourceManager<std::shared_ptr<Effect>, resource::ResourceFinder, &Resources::EffectLoader> mEffects;
    resource::ResourceManager<std::shared_ptr<Mesh>,   resource::ResourceFinder, &Resources::MeshLoader>  mMeshes;
};


} // namespace snac
} // namespace ad
