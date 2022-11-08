#pragma once


#include <renderer/VertexSpecification.h>

#include <map>


namespace ad {
namespace snac {


enum class Semantic
{
    Position,
    Normal,
    Color,
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


} // namespace snac
} // namespace ad
