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
};


struct VertexStream
{
    std::vector<VertexBufferView> mBufferViews;
    std::map<Semantic, AttributeAccessor> mSemanticToAttribute;
    IndexBufferView mIndexBufferView;
    GLsizei mVertexCount = 0;
    GLenum mPrimitiveMode = NULL;
};

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