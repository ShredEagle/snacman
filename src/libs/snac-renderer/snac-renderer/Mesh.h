#pragma once


#include <handy/StringUtilities.h>

#include <renderer/VertexSpecification.h>

#include <map>
#include <string>


namespace ad {
namespace snac {


enum class Semantic
{
    // TODO should we have separate vertex/instance semantics
    // Usally per vertex
    Position,
    Normal,
    Albedo,
    TextureCoords,
    // Usually per instance
    LocalToWorld,

    _End/* Must be last
*/
};


inline std::string to_string(Semantic aSemantic)
{
#define MAPPING_STR(semantic) case Semantic::semantic: return #semantic;
    switch(aSemantic)
    {
        MAPPING_STR(Position)
        MAPPING_STR(Normal)
        MAPPING_STR(Albedo)
        MAPPING_STR(TextureCoords)
        MAPPING_STR(LocalToWorld)
        default:
        {
            auto value = static_cast<std::underlying_type_t<Semantic>>(aSemantic);
            if(value >= static_cast<std::underlying_type_t<Semantic>>(Semantic::_End))
            {
                throw std::invalid_argument{"There are no semantics with value: " + std::to_string(value) + "."};
            }
            else
            {
                // Stop us here in a debug build
                assert(false);
                return "(Semantic#" + std::to_string(value) + ")";
            }
        }
    }
#undef MAPPING_STR
}

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
    MAPPING(TextureCoords)
    MAPPING(LocalToWorld)
    else
    {
        throw std::logic_error{
            "Unable to map shader attribute name '" + std::string{aAttributeName} + "' to a semantic."};
    }
#undef MAPPING
}


// Note: At the moment, the fact that an attribute is normalized or not
//       is hardcoded by its semantic. This is to be revisited when new need arises.
inline bool isNormalized(Semantic aSemantic)
{
    switch(aSemantic)
    {
        case Semantic::Albedo:
        {
            return true;
        }
        case Semantic::Position:
        case Semantic::Normal:
        case Semantic::TextureCoords:
        case Semantic::LocalToWorld:
        {
            return false;
        }
        default:
        {
            throw std::logic_error{
                "Semantic '" + to_string(aSemantic) + "' normalization has not been listed."};
        }
    }
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
        GLsizei mStride{0};
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
    std::map<Semantic, graphics::ClientAttribute> mAttributes;
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
