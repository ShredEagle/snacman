#pragma once


#include "Repositories.h"

#include <renderer/VertexSpecification.h>


namespace ad::renderer {


struct VertexStream;
struct InstanceStream;
struct IntrospectProgram;


graphics::VertexArrayObject prepareVAO(const VertexStream & aVertices,
                                       //const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram);


void setBufferBackedBlocks(const RepositoryUBO & aUniformBufferObjects,
                           const IntrospectProgram & aProgram);


} // namespace ad::renderer