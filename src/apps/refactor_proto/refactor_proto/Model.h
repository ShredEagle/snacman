#pragma once


#include "Commons.h" 
#include "IntrospectProgram.h" 
#include "Material.h"

#include <renderer/Texture.h>
#include <renderer/VertexSpecification.h>

#include <math/Vector.h>

// TODO this is a bit of heavy includes for the general Model file.
#include <map>
#include <set>
#include <vector>


namespace ad::renderer {


//
// GL coupled (low level)
//

struct BufferView
{
    graphics::BufferAny * mGLBuffer;
    // The stride is at the view level (not buffer).
    // This way the buffer can store heterogeneous sections, accessed by distinct views.
    GLsizei mStride;
    // The instance divisor it at the view level, not AttributeAccessor
    // The reasoning is that since the view is supposed to represent an homogeneous chunck of the buffer,
    // the divisor (like the stride) will be shared by all vertices pulled from the view.
    GLuint mInstanceDivisor = 0;
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
    IntrospectProgram * mProgram;
};


struct Effect
{
    std::vector<Technique> mTechniques;
};


struct Material
{
    // TODO currently, hardcodes the parameters type to be for phong model
    // later on, it should allow for different types of parameters (that will have to match the different shader program expectations)
    // also, we will probably load all parameters of a given type into buffers, and access them in shaders via indices (AZDO)
    std::size_t mPhongMaterialIdx = (std::size_t)-1;
    Effect * mEffect;
};


struct AttributeAccessor
{
    std::vector<BufferView>::size_type mBufferViewIndex;
    graphics::ClientAttribute mClientDataFormat;
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

//// TODO might be better as a binary flag, but this still should allow for user extension
//// (anyway, feature order should not be important)
//using FeatureSet = std::set<Feature>;

struct Part
{
    VertexStream mVertexStream;
    Material mMaterial; // TODO #matref: should probably be a "reference", as it is likely shared
    //FeatureSet mFeatures;
};


struct Object
{
    std::vector<Part> mParts;
};


///
/// Client side
///

struct Pose
{
    math::Vec<3, float> mPosition;
    float mUniformScale;

    Pose transform(Pose aNested) const
    {
        aNested.mPosition += mPosition;
        aNested.mUniformScale *= mUniformScale;
        return aNested;
    }
};


/// A good candidate to be at the interface between client and renderer lib.
/// Equivalent to a "Shape" in the shape list from AZDO talks.
struct Instance
{
    Object * mObject;
    Pose mPose;
    // TODO #matref
    std::optional<Material> mMaterialOverride;
};


struct Node
{
    Instance mInstance;
    std::vector<Node> mChildren;
};


// Loosely inspired by Godot data model, this class should probably not exist in the library
struct Storage
{
    std::vector<graphics::BufferAny> mBuffers;
    std::vector<Object> mObjects;
    std::vector<Effect> mEffects;
    std::vector<IntrospectProgram> mPrograms;
    std::vector<PhongMaterial> mPhongMaterials;
    std::vector<graphics::Texture> mTextures;
};


} // namespace ad::renderer