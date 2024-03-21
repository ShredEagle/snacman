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


/// @brief Return the byte size of an attribute.
inline GLuint getByteSize(AttributeDescription aAttribute)
{ return aAttribute.mDimension.countComponents() * graphics::getByteSize(aAttribute.mComponentType); }


Handle<graphics::BufferAny> makeBuffer(GLsizei aElementSize,
                                       GLsizeiptr aElementCount,
                                       GLenum aHint,
                                       Storage & aStorage);

/// @brief Create a GL buffer of specified size (without loading data into it).
/// @return Buffer view to the buffer.
// TODO rename (this also wraps in a BufferView)
BufferView makeBufferGetView(GLsizei aElementSize,
                             GLsizeiptr aElementCount,
                             GLuint aInstanceDivisor,
                             GLenum aHint,
                             Storage & aStorage);


Handle<ConfiguredProgram> storeConfiguredProgram(IntrospectProgram aProgram, Storage & aStorage);


/*
 * Mid-level functions to manage VertexStreams
 */

/// @brief Add a new VertexStream to storage, initializing it with the view and attributes from `aGenericStream`.
/// @param aGenericStream is intended to provide a stream containing per instance data.
Handle<VertexStream> primeVertexStream(const GenericStream & aGenericStream, 
                                       Storage & aStorage);


/// @brief Set the index buffer for `aStream`.
void setIndexBuffer(Handle<VertexStream> aStream, 
                    GLenum aIndexType,
                    Handle<graphics::BufferAny> aIndexBuffer,
                    unsigned int aIndicesCount,
                    GLintptr aBufferOffset = 0);


// TODO extend for interleaving
/// @brief Add vertex attributes to `aVertexStream`.
void addVertexAttributes(Handle<VertexStream> aVertexStream, 
                         AttributeDescription aAttribute,
                         Handle<graphics::BufferAny> aVertexBuffer,
                         unsigned int aVerticesCount,
                         GLintptr aBufferOffset = 0);


/*
 * High-level functions to manage VertexStreams
 */

/// @note High level method to provide a distinct buffer for each attribute stream at the moment (i.e. no interleaving).
Handle<VertexStream> makeVertexStream(unsigned int aVerticesCount,
                                      unsigned int aIndicesCount,
                                      GLenum aIndexType,
                                      std::span<const AttributeDescription> aBufferedStreams,
                                      Storage & aStorage,
                                      const GenericStream & aStream);

} // namespace ad::renderer