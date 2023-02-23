#include "Resources.h"

#include <arte/detail/Json.h>

#include <renderer/ShaderSource.h>
#include <renderer/utilities/FileLookup.h>

#include <fstream>


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
    // Keep the ShaderSource instance directly, not just a view to an otherwise expired temporary.
    std::vector<std::pair<const GLenum, graphics::ShaderSource>> shaders;

    Json program = Json::parse(std::ifstream{aProgram});
    graphics::FileLookup lookup{aProgram};
    for (auto [shaderStage, shaderFile] : program.items())
    {
        GLenum stageEnumerator;
        if(shaderStage == "vertex")
        {
            stageEnumerator = GL_VERTEX_SHADER;
        }
        else if (shaderStage == "fragment")
        {
            stageEnumerator = GL_FRAGMENT_SHADER;
        }
        else
        {
            SELOG(critical)("Unable to map shader stage key '{}' to a program stage.", shaderStage);
            throw std::invalid_argument{"Unhandled shader stage key."};
        }
        
        auto [inputStream, identifier] = lookup(shaderFile);
        shaders.emplace_back(
            stageEnumerator,
            graphics::ShaderSource::Preprocess(*inputStream, identifier, lookup));
    }

    SELOG(debug)("Compiling shader program from '{}', containing {} stages.", lookup.top(), shaders.size());

    return std::make_shared<Effect>(Effect{
        .mProgram = IntrospectProgram{
            graphics::makeLinkedProgram(shaders.begin(), shaders.end()),
            aProgram.filename().string(),
        }
    });
    
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
