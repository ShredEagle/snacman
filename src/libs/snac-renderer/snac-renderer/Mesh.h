#pragma once

#include "IntrospectProgram.h"
#include "Semantic.h"

#include <renderer/Shading.h>
#include <renderer/Texture.h>
#include <renderer/VertexSpecification.h>

#include <map>


namespace ad {
namespace snac {


struct Effect
{
    IntrospectProgram mProgram;
};


struct Material
{
    //graphics::Texture mTexture; // TODO extend to handle several textures
    std::shared_ptr<Effect> mEffect;
};


// We use the same model as glTF, because it is good, and because it will
// make it convenient to load glTF models.
// Note: A difference is that we do **not** store lengths in buffer, buffer view, etc.
//       This is because the data will already be loaded into the buffer by the time
//       those classes are used, and all is needed then is the count of vertices or instances.

struct BufferView
{
    graphics::VertexBufferObject mBuffer;
    GLsizei mStride{0};

    // TODO Extend the buffer view so a single buffer can store the vertex data for several meshes.
    //      This will require sharing the vertex buffer object between views.
    //      I would use shared_ptr for simplicity and safe reference counting.
    //GLuint byteOffset; // Offset from the start of the underlying buffer to the start of the view.
};


struct AttributeAccessor
{
    std::size_t mBufferViewIndex;
    graphics::ClientAttribute mAttribute;
};


struct VertexStream
{
    std::vector<BufferView> mVertexBuffers;
    std::map<Semantic, AttributeAccessor> mAttributes;
    GLsizei mVertexCount{0};
    GLenum mPrimitive;
    // TODO handle indexed rendering
    //IndexBufferObject mIndexBuffer;
};


struct Mesh
{
    VertexStream mStream;
    std::shared_ptr<Material> mMaterial;
};


struct InstanceStream
{
    template <class T_value, std::size_t N_spanExtent>
    void respecifyData(std::span<T_value, N_spanExtent> aData);

    BufferView mInstanceBuffer;
    // No need for an Accessor to indicate the buffer view, as there is only one.
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