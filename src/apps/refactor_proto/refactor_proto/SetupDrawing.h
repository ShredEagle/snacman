#pragma once


#include "Repositories.h"

#include <renderer/VertexSpecification.h>


namespace ad::renderer {


struct VertexStream;
struct InstanceStream;
struct IntrospectProgram;
struct SemanticBufferViews;


graphics::VertexArrayObject prepareVAO(const IntrospectProgram & aProgram,
                                       const VertexStream & aVertices,
                                       std::initializer_list<const SemanticBufferViews *> aExtraVertexAttributes = {});


void setBufferBackedBlocks(const IntrospectProgram & aProgram,
                           const RepositoryUBO & aUniformBufferObjects);


void setTextures(const IntrospectProgram & aProgram,
                 const RepositoryTexture & aTextures);


} // namespace ad::renderer