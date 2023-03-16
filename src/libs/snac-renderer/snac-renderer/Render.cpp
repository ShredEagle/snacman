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


void Renderer::render(const Mesh & aMesh,
                      const InstanceStream & aInstances,
                      UniformRepository & aUniforms,
                      UniformBlocks & aUniformBlocks,
                      TextureRepository & aTextures,
                      const std::vector<Technique::Annotation> & aTechniqueFilter)
{
    auto depthTest = graphics::scopeFeature(GL_DEPTH_TEST, true);

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

        graphics::ScopedBind boundVAO{mVertexArrayRepo.get(aMesh, aInstances, *program)};

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