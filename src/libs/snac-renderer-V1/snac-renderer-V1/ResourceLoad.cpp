#include "ResourceLoad.h"

#include "Cube.h"
#include "Logging.h"

#include "gltf/GltfLoad.h"

#include <arte/detail/Json.h>

#include <renderer/ShaderSource.h>
#include <renderer/utilities/FileLookup.h>

#include <spdlog/spdlog.h>

#include <fstream>


#define SELOG(level) SELOG_LG(::ad::snac::gResourceLogger, level)


namespace ad {
namespace snac {


bool attemptRecompile(Technique & aTechnique)
{
    try 
    {
        aTechnique.mProgram = loadProgram(aTechnique.mProgramFile);
        return true;
    }
    catch(graphics::ShaderCompilationError & aError)
    {
        SELOG(error)
            ("Cannot recompile program from file '{}' due to shader compilation error:\n{}",
            aTechnique.mProgramFile.string(), aError.what());
    }
    return false;
}


IntrospectProgram loadProgram(const filesystem::path & aProgram)
{
    std::vector<std::pair<const GLenum, graphics::ShaderSource>> shaders;

    // TODO factorize and make more robust (e.g. test file existence)
    Json program;
    try 
    {
        program = Json::parse(std::ifstream{aProgram});
    }
    catch (Json::parse_error &)
    {
        SELOG(critical)("Rethrowing exception from attempt to parse Json from '{}'.", aProgram.string());
        throw;
    }

    graphics::FileLookup lookup{aProgram};

    std::vector<std::string> defines;
    if (auto found = program.find("defines");
        found != program.end())
    {
        for (std::string macro : *found)
        {
            defines.push_back(std::move(macro));
        }
    }

    for (auto [shaderStage, shaderFile] : program.items())
    {
        GLenum stageEnumerator;
        if(shaderStage == "defines")
        {
            // Special case, not a shader stage
            continue;
        }
        else if(shaderStage == "vertex")
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
        graphics::ShaderSource preprocessed = graphics::ShaderSource::Preprocess(*inputStream, defines, identifier, lookup);
        //SELOG(error)("Preprocessed source for program '{}' stage '{}':\n{}",
        //             aProgram.string(),
        //             shaderStage,
        //             std::string_view{graphics::ShaderSourceView{preprocessed}});
        shaders.emplace_back(
            stageEnumerator,
            std::move(preprocessed));
    }

    SELOG(debug)("Compiling shader program from '{}', containing {} stages.", lookup.top(), shaders.size());

    return IntrospectProgram{
            graphics::makeLinkedProgram(shaders.begin(), shaders.end()),
            aProgram.filename().string()
    };
}


Technique loadTechnique(filesystem::path aProgram)
{
    return Technique{
        .mProgram = loadProgram(aProgram),
        .mProgramFile = std::move(aProgram),
    };
}


std::shared_ptr<Effect> loadTrivialEffect(Technique aTechnique)
{

    // It is not possible to list initialize a vector of move-only types.
    // see: https://stackoverflow.com/a/8468817/1027706
    auto result = std::make_shared<Effect>();
    result->mTechniques.push_back(std::move(aTechnique));
    return result;
}


std::shared_ptr<Effect> loadEffect(filesystem::path aEffectFile,
                                   Load<Technique> & aTechniqueAccess)
{
    auto result = std::make_shared<Effect>();

    Json effect = Json::parse(std::ifstream{aEffectFile});
    for (const auto & technique : effect.at("techniques"))
    {
        result->mTechniques.push_back(
            aTechniqueAccess.get(technique.at("programfile"))
        );
        if(technique.contains("annotations"))
        {
            Technique & inserted = result->mTechniques.back();
            
            for(auto [category, value] : technique.at("annotations").items())
            {
                inserted.mAnnotations.emplace(handy::StringId{category},
                                              handy::StringId{value.get<std::string>()});
            }
        }
    }

    return result;
}


Mesh loadVertices(VertexStream aVertices, std::shared_ptr<Effect> aEffect, std::string_view aName)
{
    snac::Mesh mesh{
        .mStream = std::move(aVertices),
        .mMaterial = std::make_shared<snac::Material>(snac::Material{
            .mEffect = std::move(aEffect)
        }),
        .mName = std::string{aName},
    }; 

    return mesh;
}


Mesh loadBox(math::Box<float> aBox, std::shared_ptr<Effect> aEffect, std::string_view aName)
{
    return loadVertices(makeBox(aBox), std::move(aEffect), aName);
}


Model loadModel(filesystem::path aGltf, std::shared_ptr<Effect> aEffect)
{
    snac::Model model{loadGltf(aGltf)};
    for (snac::Mesh & mesh : model.mParts)
    {
        mesh.mMaterial->mEffect = aEffect;
    }
    return model;
}


} // namespace graphics
} // namespace ad
