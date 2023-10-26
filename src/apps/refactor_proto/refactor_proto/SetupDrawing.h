#pragma once


#include "Repositories.h"

#include <renderer/VertexSpecification.h>


namespace ad::renderer {


struct VertexStream;
struct InstanceStream;
struct IntrospectProgram;
struct GenericStream;


graphics::VertexArrayObject prepareVAO(const IntrospectProgram & aProgram,
                                       const VertexStream & aVertices);


void setBufferBackedBlocks(const IntrospectProgram & aProgram,
                           const RepositoryUbo & aUniformBufferObjects);


void setTextures(const IntrospectProgram & aProgram,
                 const RepositoryTexture & aTextures);


} // namespace ad::renderer