#pragma once


#include <renderer/VertexSpecification.h>


namespace ad::renderer {


struct VertexStream;
struct InstanceStream;
struct IntrospectProgram;


graphics::VertexArrayObject prepareVAO(const VertexStream & aVertices,
                                       //const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram);


} // namespace ad::renderer