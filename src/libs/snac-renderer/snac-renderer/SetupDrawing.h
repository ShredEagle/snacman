#pragma once

#include "IntrospectProgram.h"
#include "Mesh.h"
#include "UniformParameters.h"

#include <renderer/VertexSpecification.h>

#include <map>


namespace ad {
namespace snac {


struct VertexArrayRepository
{
    const graphics::VertexArrayObject & get(const Mesh & aMesh,
                                            const InstanceStream & aInstances,
                                            const IntrospectProgram & aProgram);

    // TODO this key is dangerous: using the address as the identity
    //      this expose us to collision through address reuse.
    //      Instead, we should use values that are safe 
    //      (e.g. for a program, the value would be the attributes interface)
    using Key = std::tuple<const Mesh *, const InstanceStream *, const graphics::Program *>;
    std::map<Key, graphics::VertexArrayObject> mVAOs;
};


void setUniforms(const UniformRepository & aUniforms, const IntrospectProgram & aProgram);

void setBlocks(const UniformBlocks & aUniformBlocks, const IntrospectProgram & aProgram);


} // namespace snac
} // namespace ad