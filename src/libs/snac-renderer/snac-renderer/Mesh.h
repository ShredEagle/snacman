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
    GLsizei mVertexCount{0};
    //IndexBufferObject mIndexBuffer;
};


struct Mesh
{
    VertexStream mStream;
    //Material mMaterial;
};


struct InstanceStream
{
    template <class T_value, std::size_t N_spanExtent>
    void respecifyData(std::span<T_value, N_spanExtent> aData);

    VertexStream::VertexBuffer mInstanceBuffer;
    std::map<Semantic, VertexStream::Attribute> mAttributes;
    GLsizei mInstanceCount{0};
};


template <class T_value, std::size_t N_spanExtent>
void InstanceStream::respecifyData(std::span<T_value, N_spanExtent> aData)
{
    respecifyBuffer(mInstanceBuffer.mBuffer, aData);
    mInstanceCount = (GLsizei)aData.size();
}


} // namespace snac
} // namespace ad
