#pragma once


#include <handy/StringUtilities.h>

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
    LocalToWorld,
};


inline Semantic to_semantic(std::string_view aAttributeName)
{
    static const std::string delimiter = "_";
    std::string_view prefix, body, suffix;
    std::tie(prefix, body) = lsplit(aAttributeName, delimiter);
    std::tie(body, suffix) = lsplit(body, delimiter);

#define MAPPING(name) else if(body == #name) { return Semantic::name; }
    if(false){}
    MAPPING(Position)
    MAPPING(Normal)
    MAPPING(Albedo)
    MAPPING(LocalToWorld)
    else
    {
        throw std::logic_error{
            "Unable to map shader attribute name '" + std::string{aAttributeName} + "' to a semantic."};
    }
#undef MAPPING
}


struct VertexStream
{
    // ~glTF::accessor
    struct Attribute
    {
        std::size_t mVertexBufferIndex;
        graphics::ClientAttribute mAttribute;
    };

    // TODO generalize via a BufferView, so a VBO can contain several distinct sections.
    //      See glTF format.
    struct VertexBuffer
    {
        graphics::VertexBufferObject mBuffer;
        std::size_t mStride{0};
    };

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
