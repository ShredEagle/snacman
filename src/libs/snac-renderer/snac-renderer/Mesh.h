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

    graphics::VertexArrayObject mVertexArray;
    std::vector<graphics::VertexBufferObject> mVertexBuffers;
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
