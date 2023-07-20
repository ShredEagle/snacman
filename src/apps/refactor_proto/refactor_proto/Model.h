#pragma once


#include "Commons.h" 
#include "IntrospectProgram.h" 

#include <renderer/VertexSpecification.h>

#include <math/Vector.h>

#include <map>
#include <vector>


namespace ad::renderer {


//
// GL coupled (low level)
//

template <graphics::BufferType N_type>
struct BufferView
{
    graphics::Buffer<N_type> * mGLBuffer;
    // The stride is at the view level (not buffer).
    // This way the buffer can store heterogeneous elements.
    GLsizei mStride;
    GLintptr mOffset;
    GLsizeiptr mSize; // The size (in bytes) this buffer view has access to, starting from mOffset.
                      // Intended to be used for safety checks.
};


using VertexBufferView = BufferView<graphics::BufferType::Array>;
using IndexBufferView = BufferView<graphics::BufferType::ElementArray>;


//
// Material & Shader
//

struct Technique
{
    struct Annotation
    {
        StringKey mCategory;
        StringKey mValue;
    };

    std::map<StringKey, StringKey> mAnnotations;
    IntrospectProgram mProgram;
};


struct Effect
{
    std::vector<Technique> mTechniques;
};


struct Material
{
    Effect * mEffect;
};


struct AttributeAccessor
{
    std::vector<VertexBufferView>::size_type mBufferViewIndex;
    graphics::ClientAttribute mClientDataFormat;
    GLuint mInstanceDivisor = 0;
};


#define SEMANTIC_BUFFER_MEMBERS \
std::vector<VertexBufferView> mBufferViews; \
std::map<Semantic, AttributeAccessor> mSemanticToAttribute;

/// \note Intermediary class that holds a collection of buffer views,
/// with associated semantic and client format description.
struct SemanticBufferViews
{
    SEMANTIC_BUFFER_MEMBERS                 
};


// Not using inheritance, since it does allow designated initializers
struct VertexStream /*: public SemanticBufferViews*/
{
    SEMANTIC_BUFFER_MEMBERS                 
    IndexBufferView mIndexBufferView;
    GLsizei mVertexCount = 0;
    GLenum mPrimitiveMode = NULL;
};

#undef SEMANTIC_BUFFER_MEMBERS

struct Part
{
    VertexStream mVertexStream;
    Material mMaterial;
};


struct Object
{
    std::vector<Part> mParts;
};


struct Pose
{
    math::Vec<3, float> mPosition;
    float mUniformScale;
};


struct Instance
{
    Object * mObject;
    Pose mPose;
};


// Loosely inspired by Godot data model, this class should not exist in the library
struct Storage
{
    std::vector<graphics::VertexBufferObject> mVertexBuffers;
    std::vector<Effect> mEffects;
};


} // namespace ad::renderer