#pragma once

#include "IntrospectProgram.h"
#include "Rigging.h"
#include "Semantic.h"
#include "UniformParameters.h"

#include <handy/StringId.h>

#include <math/Box.h>

#include <renderer/Shading.h>
#include <renderer/Texture.h>
#include <renderer/VertexSpecification.h>

#include <map>
#include <optional>


namespace ad {
namespace snac {



// TODO extend with passes
struct Technique
{
    struct Annotation
    {
        handy::StringId mCategory;
        handy::StringId mValue;
    };

    std::map<handy::StringId, handy::StringId> mAnnotations;
    IntrospectProgram mProgram;
    filesystem::path mProgramFile; // TODO I am unsure if the technique (or even effect) should be hosting this path
};


struct Effect
{
    std::vector<Technique> mTechniques;
};


struct Material
{
    TextureRepository mTextures;
    UniformRepository mUniforms;
    UniformBlocks mUniformBlocks;
    std::shared_ptr<Effect> mEffect;
};


// We use the same model as glTF, because it is good, and because it will
// make it convenient to load glTF models.
// Note: A difference is that we do **not** store lengths in buffer, buffer view, etc.
//       This is because the data will already be loaded into the buffer by the time
//       those classes are used, and all is needed then is the count of vertices or instances.

struct BufferView
{
    // Important: At the moment, we make distinct buffers for each gltf buffer view.
    // This is whay the BufferView hosts the VertexBufferObject.
    // We make the bet that interleaved vertex attributes are always modeled
    // by distinct accessor on the same buffer view in gltf.
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

struct IndicesAccessor
{
    // Not using a view for the index buffer, since it cannot be interleaved with other data.
    graphics::IndexBufferObject mIndexBuffer;
    GLsizei mIndexCount{0};
    graphics::ClientAttribute mAttribute;
};


struct VertexStream
{
    template <class T_value, std::size_t N_spanExtent>
    void respecifyData(std::size_t aBufferViewIndex, std::span<T_value, N_spanExtent> aData);

    std::vector<BufferView> mVertexBuffers;
    std::map<Semantic, AttributeAccessor> mAttributes;
    std::optional<IndicesAccessor> mIndices;
    GLsizei mVertexCount{0}; // i.e. the number of elements stored in each VBO
    GLenum mPrimitive;
    math::Box<GLfloat> mBoundingBox; // TODO move out of here (or at least make optional)
};


/// \warning Correspond to the `Mesh Primitive` in glTF (2.0) lingua.
struct Mesh
{
    VertexStream mStream;
    std::shared_ptr<Material> mMaterial;
    std::string mName;
};


/// \warning Correspond to the `Mesh` in glTF (2.0) lingua.
struct Model
{
    std::vector<Mesh> mParts;
    math::Box<GLfloat> mBoundingBox;
    std::string mName;
};


std::ostream & operator<<(std::ostream & aOut, const Model & aModel);


struct InstanceStream
{
    template <class T_value, std::size_t N_spanExtent>
    void respecifyData(std::span<T_value, N_spanExtent> aData);

    BufferView mInstanceBuffer;
    // No need for an Accessor to indicate the buffer view, as there is only one.
    std::map<Semantic, graphics::ClientAttribute> mAttributes;
    GLsizei mInstanceCount{0};
};

const InstanceStream gNotInstanced{
    .mInstanceBuffer = BufferView{
        .mBuffer{graphics::VertexBufferObject::NullTag{}},
    },
    .mInstanceCount = 1,
};


template <class T_value, std::size_t N_spanExtent>
void VertexStream::respecifyData(std::size_t aBufferViewIndex, std::span<T_value, N_spanExtent> aData)
{
    // Assert the buffer view index is in range.
    assert(aBufferViewIndex < mVertexBuffers.size());
    // Assert that it is the only buffer, or the data has the size of other buffers.
    assert(mVertexBuffers.size() == 1 || mVertexCount == (int)aData.size());

    respecifyBuffer(mVertexBuffers[aBufferViewIndex].mBuffer, aData, graphics::BufferHint::StreamDraw);
    mVertexCount = (GLsizei)aData.size();
}


template <class T_value, std::size_t N_spanExtent>
void InstanceStream::respecifyData(std::span<T_value, N_spanExtent> aData)
{
    respecifyBuffer(mInstanceBuffer.mBuffer, aData, graphics::BufferHint::StreamDraw);
    mInstanceCount = (GLsizei)aData.size();
}


} // namespace snac
} // namespace ad
