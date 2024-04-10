#pragma once


#include <renderer/GL_Loader.h>

#include "Model.h"


namespace ad::renderer {


/// @brief A list of parts to be drawn, each associated to a Material.
/// It is intended to be reused accross distinct passes inside a frame (or even accross frames for static parts).
/// @note It is intended to be derived by applications if they have any more specific needs (e.g. ViewerPartList)
struct PartList
{
    static constexpr GLsizei gInvalidIdx = std::numeric_limits<GLsizei>::max();

    // SOA
    std::vector<const Part *> mParts;
    std::vector<const Material *> mMaterials;
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


/// @brief Store state and parameters required to issue a GL draw call.
struct DrawCall
{
    GLenum mPrimitiveMode = 0;
    GLenum mIndicesType = 0;

    const IntrospectProgram * mProgram;
    const graphics::VertexArrayObject * mVao;
    // TODO I am not sure having the material context here is a good idea, it feels a bit high level
    Handle<MaterialContext> mCallContext;

    // The number of consecutive DrawElementsIndirectCommand (loaded in the draw indirect buffer)
    // for this draw call.
    // Note: the offset to the first command is the sum of previous DrawCall::mDrawCount.
    GLsizei mDrawCount;
};


// TODO #model rename to DrawXxx
/// @brief The draw calls to be issued, usually valid for a given Pass.
/// @note It is intended to be derived by applications if they have any more specific needs (e.g. ViewerPassCache)
struct PassCache
{
    std::vector<DrawCall> mCalls;
    std::vector<DrawElementsIndirectCommand> mDrawCommands;
};


// 
// High level API
//

// Not implement, see prepareViewerPass for an example!
// It is very much coupled to how the application populates its instance buffer.
//PassCache preparePass(StringKey aPass,
//                      const PartList & aPartList,
//                      Storage & aStorage);


void draw(const PassCache & aPassCache,
          const RepositoryUbo & aUboRepository,
          const RepositoryTexture & aTextureRepository);


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

    // Must be defaulted in the implementation file, where Opaque definition is available 
    ~DrawEntryHelper();

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