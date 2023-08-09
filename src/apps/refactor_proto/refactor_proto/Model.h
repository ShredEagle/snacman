#pragma once


#include "Commons.h" 
#include "IntrospectProgram.h" 
#include "Material.h"

#include <renderer/Texture.h>
#include <renderer/VertexSpecification.h>

#include <math/Box.h>
#include <math/Vector.h>

// TODO this is a bit of heavy includes for the general Model file.
#include <map>
#include <set>
#include <vector>


namespace ad::renderer {


// TODO Ad 2023/08/04: #handle Define a Handle wrapper which makes sense.
// Note that a central question will be whether we have a "global" storage 
// (thus handle can be dereferenced without a storage param / without storing a pointer to storage) or not?
template <class T>
using Handle = T *;

static constexpr auto gNullHandle = nullptr;

//
// GL coupled (low level)
//

// Note: Represent an "homogeneous" chunck in a buffer, for example an array of per-vertex or per-instance data
// (with a shared stride and instance divisor).
// If we allow to store distinct logical objects in the same view, we can then use the view as an identifier
// to match VAOs (while batching). Yet this mean that a Part will need an offset into its buffer view.
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
    // TODO #bufferoffset Implement (and test) buffer offsets for both vertex and index buffers.
    // Note that it should already be taken into account by the draw command for index buffer, 
    // but it is not in place for vertex buffer (should probably be handled when setting the vertex format)
    GLintptr mOffset;
    GLsizeiptr mSize; // The size (in bytes) this buffer view has access to, starting from mOffset.
                      // Intended to be used for safety checks.
};


//
// Material & Shader
//

struct VertexStream;

/// @brief Cache of the vertex attributes configurations (i.e. VAOs) for a given program 
/// (or a set of compatible programs).
///
/// Since this cache will be at the program level, the per-program lookup is already done.
/// The remaining key to lookup is the buffer array data format / source buffer, that we map to VertexStream identity.
struct ProgramConfig
{
    struct Entry
    {
        // Note: When moving to VAB (separate format) this would become too conservative:
        // all buffer sets with the same format (attribute size, component type, relative offset) can share a VAO.
        Handle<const VertexStream> mVertexStream;
        graphics::VertexArrayObject mVao;
    };
    // Note: The cache is implemented as a vector, lookup is made by visiting elements and comparing VertexStream handles.
    std::vector<Entry> mEntries;
};



/// @brief A program with its cache of VAOs.
struct ConfiguredProgram
{
    IntrospectProgram mProgram;
    // Note: it is not ideal to "pollute" the data model with a notion of cache, but it makes access simpler.
    // Note: Distinct programs but with the same vertex attribute configuration 
    // (same semantic & type at the same attributes indices)
    // can share the same VAO, which is why we **reference** the config.
    /// @brief Provide access to the cache of VAOs for this program.
    Handle<ProgramConfig> mConfig;
};

struct Technique
{
    struct Annotation
    {
        StringKey mCategory;
        StringKey mValue;
    };

    std::map<StringKey, StringKey> mAnnotations;
    Handle<ConfiguredProgram> mConfiguredProgram;
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
struct GenericStream
{
    SEMANTIC_BUFFER_MEMBERS                 
};


// Not using inheritance, since it does allow designated initializers
struct VertexStream /*: public SemanticBufferViews*/
{
    SEMANTIC_BUFFER_MEMBERS                 
    BufferView mIndexBufferView;
    GLenum mIndicesType = NULL;
};

#undef SEMANTIC_BUFFER_MEMBERS

//// TODO might be better as a binary flag, but this still should allow for user extension
//// (anyway, feature order should not be important)
//using FeatureSet = std::set<Feature>;

struct Part
{
    Material mMaterial; // TODO #matref: should probably be a "reference", as it is likely shared
                        // on the other hand, it is currently very lightweight
    Handle<VertexStream> mVertexStream;
    GLenum mPrimitiveMode = NULL;
    GLsizei mVertexFirst = 0;
    GLsizei mVertexCount = 0;
    GLsizei mIndexFirst = 0;
    GLsizei mIndicesCount = 0;
    math::Box<GLfloat> mAabb;
    //FeatureSet mFeatures;
};


struct Object
{
    std::vector<Part> mParts;
    math::Box<GLfloat> mAabb;
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
    // TODO Ad 2023/08/09: I am not sure we want to "cache" the AABB at the node level
    // it is dependent on transformations, and the node is intended ot only live in client code.
    math::Box<GLfloat> mAabb;
};


// Loosely inspired by Godot data model, this class should probably not exist in the library
// Currently it mixes up storage of "high level" renderer abstractions (effects, objects...)
// and storage of graphics API level objects (i.e. names of textures, buffers, ...)
struct Storage
{
    // TODO Ad 2023/08/04: Change what is possible to lists (avoid relocation of elements)
    // Some are actually used for random access though, so this class needs a deep refactoring anyway
    std::list<graphics::BufferAny> mBuffers;
    std::list<Object> mObjects;
    std::list<Effect> mEffects;
    std::list<ConfiguredProgram> mPrograms;
    std::vector<PhongMaterial> mPhongMaterials;
    std::vector<graphics::Texture> mTextures;
    std::list<VertexStream> mVertexStreams;
    std::list<ProgramConfig> mProgramConfigs;
};


} // namespace ad::renderer