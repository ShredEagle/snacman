#pragma once

#include "Camera.h"
#include "Mesh.h"
#include "SetupDrawing.h"
#include "UniformParameters.h"

#include <renderer/GL_Loader.h>


namespace ad {
namespace snac {


inline void clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


const IntrospectProgram * findTechnique(
    const Material & aMaterial,
    const std::vector<Technique::Annotation> & aTechniqueFilter);


struct ProgramSetup
{
    UniformRepository mUniforms;
    UniformBlocks mUniformBlocks;
    TextureRepository mTextures;
};


class Renderer
{
public:
    // TODO It should ideally be possible to call setupProgram several times on the same program
    // **while** having consolidated warnings accross the calls. 
    // This probably means that the warnings would be issued at draw call time. This is not done yet.
    const IntrospectProgram & setupProgram(
        std::string_view aPassName,
        const IntrospectProgram & aProgram,
        const UniformRepository & aUniforms,
        const UniformBlocks & aUniformBlocks,
        const TextureRepository & aTextures);

    const IntrospectProgram & setupProgram(
        std::string_view aPassName,
        const IntrospectProgram & aProgram,
        const ProgramSetup & aSetup)
    { 
        return setupProgram(aPassName, aProgram, aSetup.mUniforms, aSetup.mUniformBlocks, aSetup.mTextures); 
    }

    void draw(
        const VertexStream & aVertices,
        const InstanceStream & aInstances,
        const IntrospectProgram & aProgram);

    void resetRepositories()
    {
        mVertexArrayRepo= VertexArrayRepository{};
        mWarningRepo = WarningRepository{};
    }

private:
    VertexArrayRepository mVertexArrayRepo;
    WarningRepository mWarningRepo;
};


class Pass
{
public:
    // TODO this is probably too specific an approach. But it avoids a lot of templating for the time being.
    struct Visual
    {
        const Mesh * mMesh;
        const InstanceStream * mInstances;
    };

    Pass(std::string aName, std::vector<Technique::Annotation> aTechniqueFilter = {}) :
        mName{std::move(aName)},
        mFilter{std::move(aTechniqueFilter)}
    {}

    void draw(std::span<const Visual> aVisuals, Renderer & aRenderer, ProgramSetup & aSetup) const;

    void draw(const Mesh & aMesh, const InstanceStream & aInstances, Renderer & aRenderer, ProgramSetup & aSetup) const;

    void draw(const Mesh & aMesh, Renderer & aRenderer, ProgramSetup & aSetup) const
    { 
        draw(aMesh, gNotInstanced, aRenderer, aSetup);
    }

private:
    std::string mName;
    std::vector<Technique::Annotation> mFilter;
};


/// \deprecated
class RendererDeprecated
{
public:
    void render(const Mesh & aMesh,
                const InstanceStream & aInstances,
                UniformRepository & aUniforms,
                UniformBlocks & aUniformBlocks,
                TextureRepository & aTextures,
                const std::vector<Technique::Annotation> & aTechniqueFilter = {});

    void render(const Mesh & aMesh,
                const InstanceStream & aInstances,
                UniformRepository & aUniforms,
                UniformBlocks & aUniformBlocks,
                const std::vector<Technique::Annotation> & aTechniqueFilter = {})
    { 
        TextureRepository textureRepo;
        render(aMesh, aInstances, aUniforms, aUniformBlocks, textureRepo, aTechniqueFilter);
    }

private:
    VertexArrayRepository mVertexArrayRepo;
    WarningRepository mWarningRepo;
};


} // namespace snac
} // namespace ad