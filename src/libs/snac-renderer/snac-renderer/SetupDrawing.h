#pragma once

#include "Mesh.h"

#include <renderer/Shading.h>
#include <renderer/VertexSpecification.h>

#include <map>


namespace ad {
namespace snac {


struct VertexArrayRepository
{
    const graphics::VertexArrayObject & get(const Mesh & aMesh,
                                                const InstanceStream & aInstances,
                                                const graphics::Program & aProgram);

    // TODO this key is dangerous: using the address as the identity
    //      this expose us to collision through address reuse.
    //      Instead, we should use values that are safe 
    //      (e.g. for a program, the value would be the attributes interface)
    using Key = std::tuple<const Mesh *, const InstanceStream *, const graphics::Program *>;
    std::map<Key, graphics::VertexArrayObject> mVAOs;
};


} // namespace snac
} // namespace ad