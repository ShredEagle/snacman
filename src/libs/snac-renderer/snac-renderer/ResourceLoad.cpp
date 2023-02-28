#include "ResourceLoad.h"

#include "Cube.h"
#include "Logging.h"

#include "gltf/GltfLoad.h"

#include <arte/detail/Json.h>

#include <renderer/ShaderSource.h>
#include <renderer/utilities/FileLookup.h>

#include <fstream>


#define SELOG(level) SELOG_LG(::ad::snac::gResourceLogger, level)


namespace ad {
namespace snac {


IntrospectProgram loadProgram(filesystem::path aProgram)
{
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

    return IntrospectProgram{
            graphics::makeLinkedProgram(shaders.begin(), shaders.end()),
            aProgram.filename().string()
    };
}

std::shared_ptr<Effect> loadEffect(filesystem::path aProgram)
{

    auto result = std::make_shared<Effect>(Effect{
        .mTechniques{},
    });

    result->mTechniques.push_back(Technique{
        .mProgram = loadProgram(aProgram),
        .mProgramFile = aProgram,
    });

    return result;
}

Mesh loadCube(std::shared_ptr<Effect> aEffect)
{
    snac::Mesh mesh{
        .mStream = makeCube(),
        .mMaterial = std::make_shared<snac::Material>(snac::Material{
            .mEffect = std::move(aEffect)
        }),
        .mName = "procedural_cube",
    }; 

    return mesh;
}


Mesh loadModel(filesystem::path aGltf, std::shared_ptr<Effect> aEffect)
{
    snac::Mesh mesh{loadGltf(aGltf)};
    mesh.mMaterial->mEffect = std::move(aEffect);
    return mesh;
}


} // namespace graphics
} // namespace ad
