#include "Render.h"

#include <renderer/ScopeGuards.h>
#include <renderer/Uniforms.h>


namespace ad {
namespace snac {


void Renderer::render(const Mesh & aMesh,
                      const InstanceStream & aInstances,
                      const UniformRepository & aUniforms,
                      const UniformBlocks & aUniformBlocks)
{
    auto depthTest = graphics::scopeFeature(GL_DEPTH_TEST, true);

    const IntrospectProgram & program = aMesh.mMaterial->mEffect->mProgram;
    setUniforms(aUniforms, program);
    setBlocks(aUniformBlocks, program);

    graphics::ScopedBind boundVAO{mVertexArrayRepo.get(aMesh, aInstances, program)};

    graphics::use(program);
    glDrawArraysInstanced(aMesh.mStream.mPrimitive, 0, aMesh.mStream.mVertexCount, aInstances.mInstanceCount);
}


} // namespace snac
} // namespace ad