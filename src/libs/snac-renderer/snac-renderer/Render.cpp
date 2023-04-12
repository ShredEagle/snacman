#include "Render.h"

#include "Logging.h"

#include <renderer/ScopeGuards.h>
#include <renderer/Uniforms.h>


#define SELOG(level) SELOG_LG(gRenderLogger, level)


namespace ad {
namespace snac {


const IntrospectProgram * findTechnique(
    const Material & aMaterial,
    const std::vector<Technique::Annotation> & aTechniqueFilter)
{
    for(const auto & technique : aMaterial.mEffect->mTechniques)
    {
        if(std::all_of(
            aTechniqueFilter.begin(), aTechniqueFilter.end(),
            [&](const Technique::Annotation & required)
            {
                auto found = technique.mAnnotations.find(required.mCategory);
                return found != technique.mAnnotations.end() 
                    && found->second == required.mValue;
            }))
        {
            return &technique.mProgram;
        }
    }
    return static_cast<const IntrospectProgram *>(nullptr);
}


const IntrospectProgram & Renderer::setupProgram(
    std::string_view aPassName,
    const IntrospectProgram & aProgram,
    const UniformRepository & aUniforms,
    const UniformBlocks & aUniformBlocks,
    const TextureRepository & aTextures)
{
    WarningRepository::WarnedUniforms & warned = mWarningRepo.get(aPassName, aProgram);
    setUniforms(aUniforms, aProgram, warned);
    setTextures(aTextures, aProgram, warned);
    setBlocks(aUniformBlocks, aProgram);
    return aProgram;
}


void Renderer::draw(
    const VertexStream & aVertices,
    const InstanceStream & aInstances,
    const IntrospectProgram & aProgram)
{
    graphics::ScopedBind boundVAO{mVertexArrayRepo.get(aVertices, aInstances, aProgram)};

    graphics::ScopedBind usedProgram(aProgram);
    if(aVertices.mIndices)
    {
        // Note: Range commands (providing a range of possible index values) are no longer usefull
        // see: https://computergraphics.stackexchange.com/a/6199/11110
        glDrawElementsInstanced(aVertices.mPrimitive,
                                aVertices.mIndices->mIndexCount,
                                aVertices.mIndices->mAttribute.mComponentType,
                                (void*)aVertices.mIndices->mAttribute.mOffset,
                                aInstances.mInstanceCount);
    }
    else
    {
        glDrawArraysInstanced(aVertices.mPrimitive,
                              0, 
                              aVertices.mVertexCount,
                              aInstances.mInstanceCount);
    }
}


void Pass::draw(std::span<const Visual> aVisuals, Renderer & aRenderer, ProgramSetup & aSetup) const
{
    // The naive algorithm, should be refined by sorting similarities.
    for (const auto & [mesh, instances] : aVisuals)
    {
        draw(*mesh, *instances, aRenderer, aSetup);
    }
}


void Pass::draw(const Mesh & aMesh, const InstanceStream & aInstances, Renderer & aRenderer, ProgramSetup & aSetup) const
{
    assert(aMesh.mMaterial);
    if(const IntrospectProgram * program = findTechnique(*aMesh.mMaterial, mFilter);
        program != nullptr)
    {
        // TODO Is there a better way to handle several source for uniform values
        auto scopeUniformPush = aSetup.mUniforms.push(aMesh.mMaterial->mUniforms);
        auto scopeUniformBlocksPush = aSetup.mUniformBlocks.push(aMesh.mMaterial->mUniformBlocks);
        auto scopeTexturePush = aSetup.mTextures.push(aMesh.mMaterial->mTextures);
        aRenderer.setupProgram(mName, *program, aSetup);

        aRenderer.draw(aMesh.mStream, aInstances, *program);
    }
}


void RendererDeprecated::render(const Mesh & aMesh,
                      const InstanceStream & aInstances,
                      UniformRepository & aUniforms,
                      UniformBlocks & aUniformBlocks,
                      TextureRepository & aTextures,
                      const std::vector<Technique::Annotation> & aTechniqueFilter)
{
    if (const IntrospectProgram * program = findTechnique(*aMesh.mMaterial, aTechniqueFilter);
        program != nullptr)
    {
        WarningRepository::WarnedUniforms & warned = mWarningRepo.get("<deprecated-renderer-pass>", *program);
        // TODO Is there a better way to handle several source for uniform values
        auto scopeUniformPush = aUniforms.push(aMesh.mMaterial->mUniforms);
        setUniforms(aUniforms, *program, warned);
        auto scopeTexturePush = aTextures.push(aMesh.mMaterial->mTextures);
        setTextures(aTextures, *program, warned);
        setBlocks(aUniformBlocks, *program);

        graphics::ScopedBind boundVAO{mVertexArrayRepo.get(aMesh.mStream, aInstances, *program)};

        graphics::ScopedBind usedProgram(*program);
        if(aMesh.mStream.mIndices)
        {
            // Note: Range commands (providing a range of possible index values) are no longer usefull
            // see: https://computergraphics.stackexchange.com/a/6199/11110
            glDrawElementsInstanced(aMesh.mStream.mPrimitive,
                                    aMesh.mStream.mIndices->mIndexCount,
                                    aMesh.mStream.mIndices->mAttribute.mComponentType,
                                    (void*)aMesh.mStream.mIndices->mAttribute.mOffset,
                                    aInstances.mInstanceCount);
        }
        else
        {
            glDrawArraysInstanced(aMesh.mStream.mPrimitive,
                                  0, 
                                  aMesh.mStream.mVertexCount,
                                  aInstances.mInstanceCount);
        }
    }
    else
    {
        SELOG(error)("No technique matching the filters, skip drawing.");
    }
}


} // namespace snac
} // namespace ad
