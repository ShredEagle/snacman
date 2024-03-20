#pragma once

#include "Model.h"


namespace ad::renderer {


struct AttributeDescription
{
    Semantic mSemantic;
    // To replace wih graphics::ClientAttribute if we support interleaved buffers (i.e. with offset)
    graphics::AttributeDimension mDimension;
    GLenum mComponentType;   // data individual components' type.
};


// TODO review the usefulness of aVertexStream last argument
/// @note Provide a distinct buffer for each attribute stream at the moment (i.e. no interleaving).
/// @param aVertexStream If not nullptr, create views into this vertex stream buffers instead of creating
/// new buffers.
Handle<VertexStream> makeVertexStream(unsigned int aVerticesCount,
                                      unsigned int aIndicesCount,
                                      std::span<const AttributeDescription> aBufferedStreams,
                                      Storage & aStorage,
                                      const GenericStream & aStream,
                                      // This is probably only useful to create debug data buffers
                                      // (as there is little sense to not reuse the existing views)
                                      const VertexStream * aVertexStream = nullptr);


/// @brief Create a GL buffer of specified size (without loading data into it).
/// @return Buffer view to the buffer.
BufferView createBuffer(GLsizei aElementSize,
                        GLsizeiptr aElementCount,
                        GLuint aInstanceDivisor,
                        GLenum aHint,
                        Storage & aStorage);


Handle<ConfiguredProgram> storeConfiguredProgram(IntrospectProgram aProgram, Storage & aStorage);


} // namespace ad::renderer