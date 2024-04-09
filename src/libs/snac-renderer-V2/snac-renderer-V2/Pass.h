#pragma once


#include <renderer/GL_Loader.h>

#include <snac-renderer-V2/Model.h>


namespace ad::renderer {


// TODO Ad 2024/04/09: Seems to be application dependent...
/// @brief A list of parts to be drawn, each associated to a Material and a transformation.
/// It is intended to be reused accross distinct passes inside a frame (or even accross frames for static parts).
struct PartList
{
    static constexpr GLsizei gInvalidIdx = std::numeric_limits<GLsizei>::max();

    // The individual renderer::Objects transforms. Several parts might index to the same transform.
    // (Same way several parts might index the same material parameters)
    std::vector<math::AffineMatrix<4, GLfloat>> mInstanceTransforms;

    // All the palettes are concatenated in a single unidimensionnal container.
    std::vector<Rig::Pose> mRiggingPalettes;

    // SOA
    std::vector<const Part *> mParts;
    std::vector<const Material *> mMaterials;
    std::vector<GLsizei> mTransformIdx;
    std::vector<GLsizei> mPaletteOffset; // the first bone index for a each part in the global matrix palette
};


/// @brief Associate an integer key to a the index of a Part in a PartList.
/// It allows to sort an array with one entry for each part,
/// by manually composing a key by sort dimensions.
/// @see https://realtimecollisiondetection.net/blog/?p=86
struct PartDrawEntry
{
    using Key = std::uint64_t;
    static constexpr Key gInvalidKey = std::numeric_limits<Key>::max();

    /// @brief Order purely by key.
    /// @param aRhs The other entry, whose key will be compared against this key.
    /// @return `true` if this key is strictly smaller than aRhs', `false` otherwise.
    bool operator<(const PartDrawEntry & aRhs) const
    {
        return mKey < aRhs.mKey;
    }

    Key mKey = 0;
    std::size_t mPartListIdx;
};


/// @brief Entry to populate the GL_DRAW_INDIRECT_BUFFER used with indexed (glDrawElements) geometry.
struct DrawElementsIndirectCommand
{
    GLuint  mCount;
    GLuint  mInstanceCount;
    GLuint  mFirstIndex;
    GLuint  mBaseVertex;
    GLuint  mBaseInstance;
};


// NOTE Ad 2024/04/09: Might also be application dependent
/// @brief Entry to populate the instance buffer (attribute divisor == 1).
/// Each instance (mapping to a `Part` in client data model) has a pose and a material.
// TODO Ad 2024/03/20: Why does it duplicate Loader.h InstanceData? (modulo the alias types)
struct DrawInstance
{
    GLsizei mInstanceTransformIdx; // index in the instance UBO
    GLsizei mMaterialIdx;
    GLsizei mMatrixPaletteOffset;
};


/// @brief Store state and parameters required to issue a GL draw call.
struct DrawCall
{
    GLenum mPrimitiveMode = 0;
    GLenum mIndicesType = 0;

    const IntrospectProgram * mProgram;
    const graphics::VertexArrayObject * mVao;
    // TODO I am not sure having the material context here is a good idea, it feels a bit high level
    Handle<MaterialContext> mCallContext;

    GLsizei mPartCount;
};


// TODO rename to DrawXxx
struct PassCache
{
    std::vector<DrawCall> mCalls;

    // SOA at the moment, one entry per part
    std::vector<DrawElementsIndirectCommand> mDrawCommands;
    std::vector<DrawInstance> mDrawInstances; // Intended to be loaded as a GL instance buffer
};


// 
// High level API
//

/// @brief From a PartList, generates the PassCache for a given pass.
/// @param aPass Pass name.
/// @param aPartList The PartList that should be rendered.
PassCache preparePass(StringKey aPass,
                      const PartList & aPartList,
                      Storage & aStorage);


//
// Low level API
//

// NOTE Ad 2024/04/09: I am not fan of having generateDrawEntries as part of a class.
// Yet, for the moment we rely on some hackish ResourceIdMap to get the integer part of the sorting key
// and those maps state must be maintained to generate the draw call by lookup.
// TODO Ad 2024/04/09: #handle If we make handle indices into storage (or light wrapper around GL names)
// we might be able to use their values directly as integer part of the key, and get rid of this struct.
struct DrawEntryHelper
{
    DrawEntryHelper();

    /// @brief Returns an array with one DrawEntry per-part in the input PartList.
    /// The DrawEntries can be sorted in order to minimize state changes.
    std::vector<PartDrawEntry> generateDrawEntries(StringKey aPass,
                                                   const PartList & aPartList,
                                                   Storage & aStorage);

    DrawCall generateDrawCall(const PartDrawEntry & aEntry,
                              const Part & aPart,
                              const VertexStream & aVertexStream);

    struct Opaque;
    std::unique_ptr<Opaque> mImpl;
};

Handle<ConfiguredProgram> getProgramForPass(const Effect & aEffect, StringKey aPassName);

Handle<graphics::VertexArrayObject> getVao(const ConfiguredProgram & aProgram,
                                           const Part & aPart,
                                           Storage & aStorage);

} // namespace ad::renderer