#pragma once

// TODO might become useless once Resources itselfs become the Load<> implementer
#include <snac-renderer/ResourceLoad.h>

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
struct Model;
template <class T_renderer> class RenderThread;

constexpr unsigned int gDefaultPixelHeight = 64;

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

    std::shared_ptr<Model> getModel(filesystem::path aModel);

    std::shared_ptr<Font> getFont(filesystem::path aFont,
                                  unsigned int aPixelHeight = gDefaultPixelHeight,
                                  filesystem::path aEffect = "effects/Text.sefx");

    /// \warning At the moment: intended to be called only from the thread where OpenGL context is active.
    std::shared_ptr<Effect> getShaderEffect(filesystem::path aEffect);

    auto find(const filesystem::path & aPath) const
    { return mFinder.find(aPath); }

    // TODO might become useless once Resources itselfs become the Load<> implementer
    snac::TechniqueLoader getTechniqueLoader() const
    { return {mFinder}; }

    const arte::Freetype & getFreetype()
    { return mFreetype; }
    
    void recompilePrograms();

private:
    static std::shared_ptr<Font> FontLoader(
        filesystem::path aFont, 
        unsigned int aPixelHeight,
        filesystem::path aEffect,
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);

    static std::shared_ptr<Effect> EffectLoader(
        filesystem::path aProgram, 
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);

    static std::shared_ptr<Model> ModelLoader(
        filesystem::path aModel, 
        RenderThread<snacgame::Renderer> & aRenderThread,
        Resources & aResources);
    
    // There is a smelly circular dependency in this design:
    // Resources knows the RenderThread to request OpenGL related resource loading
    // and the RenderThread uses Resources to load dependent resources (e.g. loading the texture from a model)
    resource::ResourceFinder mFinder;
    arte::Freetype & mFreetype;
    RenderThread<snacgame::Renderer> & mRenderThread;
    
    std::shared_ptr<Model> mCube = nullptr;
    resource::ResourceManager<std::shared_ptr<Font>,   resource::ResourceFinder, &Resources::FontLoader>   mFonts;
    resource::ResourceManager<std::shared_ptr<Effect>, resource::ResourceFinder, &Resources::EffectLoader> mEffects;
    resource::ResourceManager<std::shared_ptr<Model>,   resource::ResourceFinder, &Resources::ModelLoader> mModels;
};


} // namespace snac
} // namespace ad
