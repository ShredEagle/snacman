#include "Render.h"

#include <renderer/ScopeGuards.h>
#include <renderer/Uniforms.h>


namespace ad {
namespace snac {


void Renderer::render(const Mesh & aMesh,
                      const InstanceStream & aInstances,
                      UniformRepository aUniforms,
                      const UniformBlocks & aUniformBlocks)
{
    const IntrospectProgram & program = aMesh.mMaterial->mEffect->mProgram;

    // TODO Is there a better way to handle several source for uniform values
    aUniforms.merge(UniformRepository{aMesh.mMaterial->mUniforms});
    setUniforms(aUniforms, program, mWarningRepo.get(aMesh, program));
    setTextures(aMesh.mMaterial->mTextures, program);
    setBlocks(aUniformBlocks, program);

    graphics::ScopedBind boundVAO{mVertexArrayRepo.get(aMesh, aInstances, program)};

    graphics::use(program);
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


} // namespace snac
} // namespace ad
