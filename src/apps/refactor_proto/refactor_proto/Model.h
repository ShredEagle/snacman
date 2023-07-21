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

struct BufferView
{
    graphics::BufferAny * mGLBuffer;
    // The stride is at the view level (not buffer).
    // This way the buffer can store heterogeneous elements.
    GLsizei mStride;
    GLintptr mOffset;
    GLsizeiptr mSize; // The size (in bytes) this buffer view has access to, starting from mOffset.
                      // Intended to be used for safety checks.
};


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
    std::vector<BufferView>::size_type mBufferViewIndex;
    graphics::ClientAttribute mClientDataFormat;
    GLuint mInstanceDivisor = 0;
};


#define SEMANTIC_BUFFER_MEMBERS \
std::vector<BufferView> mVertexBufferViews; \
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
    GLsizei mVertexCount = 0;
    GLenum mPrimitiveMode = NULL;
    BufferView mIndexBufferView;
    GLenum mIndicesType = NULL;
    GLsizei mIndicesCount = 0;
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


// Loosely inspired by Godot data model, this class should not exist in the library
struct Storage
{
    std::vector<graphics::BufferAny> mBuffers;
    std::vector<Effect> mEffects;
};


///
/// Client side
///

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


struct Node
{
    Instance mInstance;
    std::vector<Node> mChildren;
};

} // namespace ad::renderer