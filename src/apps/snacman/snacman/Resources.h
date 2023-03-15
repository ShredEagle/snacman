#pragma once


// Note: this is coupling the Resource class to the specifics of snacgame. 
// But I do not want to template Resource on the renderer...
#include <resource/ResourceManager.h>
#include <resource/ResourceFinder.h>   // for ResourceFinder
#include <arte/Freetype.h>             // for Freetype

#include <filesystem>                  // for path
#include <memory>                      // for shared_ptr
#include <utility>                     // for move


namespace ad {

namespace snacgame { class Renderer; }

namespace snac {

struct Effect;
struct Font;
struct Mesh;
template <class T_renderer> class RenderThread;

constexpr unsigned int gDefaultPixelHeight = 64;

class Resources
{
public:
    Resources(resource::ResourceFinder aFinder, RenderThread<snacgame::Renderer> & aRenderThread) :
        mRenderThread{aRenderThread},
        mFinder{std::move(aFinder)}
    {}

    std::shared_ptr<Mesh> getShape(filesystem::path aShape);

    std::shared_ptr<Font> getFont(filesystem::path aFont, unsigned int aPixelHeight = gDefaultPixelHeight);

    /// \warning At the moment: intended to be called only from the thread where OpenGL context is active.
    std::shared_ptr<Effect> getShaderEffect(filesystem::path aProgram);

    auto find(const filesystem::path & aPath) const
    { return mFinder.find(aPath); }

    const arte::Freetype & getFreetype()
    { return mFreetype; }

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
    RenderThread<snacgame::Renderer> & mRenderThread;
    resource::ResourceFinder mFinder;

    // Must outlive all FontFaces: 
    // * it must outlive the EntityManager (which might contain Freetype FontFaces)
    // * it must outlive the resource managers holding fonts
    arte::Freetype mFreetype;
    
    std::shared_ptr<Mesh> mCube = nullptr;
    resource::ResourceManager<std::shared_ptr<Font>,   resource::ResourceFinder, &Resources::FontLoader>   mFonts;
    resource::ResourceManager<std::shared_ptr<Effect>, resource::ResourceFinder, &Resources::EffectLoader> mEffects;
    resource::ResourceManager<std::shared_ptr<Mesh>,   resource::ResourceFinder, &Resources::MeshLoader>  mMeshes;
};


} // namespace snac
} // namespace ad
