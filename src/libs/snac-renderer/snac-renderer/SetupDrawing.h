#pragma once

#include "IntrospectProgram.h"
#include "Mesh.h"
#include "UniformParameters.h"

#include <handy/StringId.h>

#include <renderer/VertexSpecification.h>

#include <map>
#include <set>


namespace ad {
namespace snac {


struct VertexArrayRepository
{
    const graphics::VertexArrayObject & get(const VertexStream & aVertices,
                                            const InstanceStream & aInstances,
                                            const IntrospectProgram & aProgram);

    // TODO this key is dangerous: using the address as the identity
    //      this expose us to collision through address reuse.
    //      Instead, we should use values that are safe 
    //      (e.g. for a program, the value would be the attributes interface)
    using Key = std::tuple<const VertexStream *, const InstanceStream *, const graphics::Program *>;
    std::map<Key, graphics::VertexArrayObject> mVAOs;
};


/// \brief This class is a workaround for the problem of recurring uniforms warning.
/// (at each frame, when a uniform is not bound, the same warning is emitted)
/// \todo Identifying the logical situation where "the warning has already been emitted" is not trivial.
/// For the moment, we use the mesh and the program to be synonymous of the logical situation, but this
/// might need reworking.
struct WarningRepository
{
    // Note: also used for textures
    using WarnedUniforms = std::set<std::string>;

    WarningRepository::WarnedUniforms & get(std::string_view aPassName,
                                            const IntrospectProgram & aProgram);

    using Key = std::pair<handy::StringId /*pass*/, const graphics::Program *>;
    std::map<Key, WarnedUniforms> mWarnings;
};


void setUniforms(const UniformRepository & aUniforms,
                 const IntrospectProgram & aProgram,
                 WarningRepository::WarnedUniforms & aWarnedUniforms);

void setTextures(const TextureRepository & aTextures,
                 const IntrospectProgram & aProgram,
                 WarningRepository::WarnedUniforms & aWarnedUniforms);

void setBlocks(const UniformBlocks & aUniformBlocks, const IntrospectProgram & aProgram);


} // namespace snac
} // namespace ad