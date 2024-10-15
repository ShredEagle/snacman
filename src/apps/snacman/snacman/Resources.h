#pragma once

#include "simulations/snacgame/Renderer.h"  // for Renderer

// TODO might become useless once Resources itselfs become the Load<> implementer
#include <snac-renderer-V1/ResourceLoad.h>
#include "entity/Entity.h"
#include "entity/HandleKey.h"

// Note: this is coupling the Resource class to the specifics of snacgame. 
// But I do not want to template Resource on the renderer...
#include <resource/ResourceManager.h>
#include <resource/ResourceFinder.h>   // for ResourceFinder
#include <arte/Freetype.h>             // for Freetype

#include <filesystem>                  // for path
#include <memory>                      // for shared_ptr
#include <utility>                     // for move


namespace ad {


namespace snac {

struct Effect;
struct Font;
struct FontSerialData
{
    filesystem::path mFontPath;
    unsigned int mPixelHeight;
    filesystem::path mEffectPath;
};
struct Model;
struct ModelData
{
    filesystem::path mModelPath;
    filesystem::path mEffectPath;
};
template <class T_renderer> class RenderThread;

constexpr unsigned int gDefaultPixelHeight = 64;

// TODO Ad 2024/02/14: This class should at least be split in 2:
// * The high level client-facing API, to request (async?) loads on the RenderThread
// * And underlying synchronous loader, which does not need to know about RenderThread,
//   but that RenderThread uses.
// Overall, the whole resource system needs a massive redesign
class Resources
{
public:
    Resources(resource::ResourceFinder aFinder,
              arte::Freetype & aFreetype,
              RenderThread<snacgame::Renderer_t> & aRenderThread) :
        mFinder{std::move(aFinder)},
        mFreetype{aFreetype},
        mRenderThread{aRenderThread},
        mResources_V2{mFinder}
    {}

    // Just to log it
    ~Resources();

    /// @warning It is an error to load the same model with distinct effects.
    /// At the moment, the effect path is not considered when looking up in the map,
    /// so the model will always have the effect it was loaded with the first time.
    snacgame::Renderer_t::Handle_t<const renderer::Object> getModel(filesystem::path aModel, filesystem::path aEffect);
    ModelData getModelDataFromResource(snacgame::Renderer_t::Handle_t<const renderer::Object> aResource)
    { return mModelDataFromResource.at(aResource); }

    // TODO Ad 2024/03/28: #RV2 #text
    std::shared_ptr<Font> getFont(filesystem::path aFont,
                                  unsigned int aPixelHeight = gDefaultPixelHeight,
                                  filesystem::path aEffect = "effects/Text.sefx");
    FontSerialData getFontDataFromResource(std::shared_ptr<Font> & aResource)
    { return mFontDataFromResource.at(aResource); }
    ent::Handle<ent::Entity> getBlueprint(filesystem::path aBpFile);

    /// \warning At the moment: intended to be called only from the thread where OpenGL context is active.
    // TODO Ad 2024/03/28: #RV2 #decommissionRV1 Finish porting to V2 resources (not even sure we need a this anymore)
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
        RenderThread<snacgame::Renderer_t> & aRenderThread,
        Resources & aResources);
    std::unordered_map<std::shared_ptr<Font>, FontSerialData> mFontDataFromResource;

    static std::shared_ptr<Effect> EffectLoader(
        filesystem::path aProgram, 
        RenderThread<snacgame::Renderer_t> & aRenderThread,
        Resources & aResources);

    static snacgame::Renderer_t::Handle_t<const renderer::Object> ModelLoader(
        filesystem::path aModel, 
        filesystem::path aEffect,
        RenderThread<snacgame::Renderer_t> & aRenderThread,
        snacgame::Resources_V2 & aResources);
    std::unordered_map<snacgame::Renderer_t::Handle_t<const renderer::Object>, ModelData> mModelDataFromResource;

    static ent::Handle<ent::Entity> BpLoader(
        filesystem::path aBpFile, 
        ent::EntityManager & aWorld,
        Resources & aResources);
    
    // There is a smelly circular dependency in this design:
    // Resources knows the RenderThread to request OpenGL related resource loading
    // and the RenderThread uses Resources to load dependent resources (e.g. loading the texture from a model)
    resource::ResourceFinder mFinder;
    arte::Freetype & mFreetype;
    RenderThread<snacgame::Renderer_t> & mRenderThread;

    snacgame::Resources_V2 mResources_V2;
    
    std::shared_ptr<Model> mCube = nullptr;
    resource::ResourceManager<std::shared_ptr<Font>,   resource::ResourceFinder, &Resources::FontLoader>   mFonts;
    resource::ResourceManager<std::shared_ptr<Effect>, resource::ResourceFinder, &Resources::EffectLoader> mEffects;
    resource::ResourceManager<snacgame::Renderer_t::Handle_t<const renderer::Object>,   resource::ResourceFinder, &Resources::ModelLoader> mModels;
    resource::ResourceManager<ent::HandleKey<ent::Entity>,   resource::ResourceFinder, &Resources::ModelLoader> mBlueprints;
};


} // namespace snac
} // namespace ad
