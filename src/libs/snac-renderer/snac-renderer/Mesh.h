#pragma once


#include <renderer/VertexSpecification.h>

#include <map>


namespace ad {
namespace snac {


enum class Semantic
{
    // TODO should we have separate vertex/instance semantics
    // Usally per vertex
    Position,
    Normal,
    Albedo,
    // Usually per instance
    LocalToWorld
};


struct VertexStream
{
    struct Attribute
    {
        std::size_t mVertexBufferIndex;
        graphics::ClientAttribute mAttribute;
    };

    // TODO should be generalized via a BufferView, so a VBO can contain several diffrent sections.
    struct VertexBuffer
    {
        graphics::VertexBufferObject mBuffer;
        std::size_t mStride{0};
    };

    graphics::VertexArrayObject mVertexArray;
    std::vector<VertexBuffer> mVertexBuffers;
    std::map<Semantic, Attribute> mAttributes;
    GLsizei mVertexCount;
    //IndexBufferObject mIndexBuffer;
};


struct Mesh
{
    VertexStream mStream;
    //Material mMaterial;
};


struct InstanceStream
{
    VertexStream::VertexBuffer mInstanceBuffer;
    std::map<Semantic, VertexStream::Attribute> mAttributes;
    GLsizei mInstanceCount;
};


} // namespace snac
} // namespace ad
