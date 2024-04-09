#include "Pass.h"

#include "Profiling.h"
#include "SetupDrawing.h"


namespace ad::renderer {

    
namespace {


    // TODO Ad 2023/11/28: Move to some low-level masking helpers lib
    /// @brief Make a 64 bit mask, with `aBitCount`'th low-order bits set to 1.
    constexpr std::uint64_t makeMask(unsigned int aBitCount)
    {
        assert(aBitCount <= 64);
        return (1 << aBitCount) - 1;
    }


    /// @brief Find the program satisfying all annotations.
    /// @return The first program in aEffect for which all annotations are matching (i.e. present and same value),
    /// or a null handle otherwise.
    template <class T_iterator>
    Handle<ConfiguredProgram> getProgram(const Effect & aEffect,
                                         T_iterator aAnnotationsBegin, T_iterator aAnnotationsEnd)
    {
        for (const Technique & technique : aEffect.mTechniques)
        {
            if(std::all_of(aAnnotationsBegin, aAnnotationsEnd,
                    [&technique](const Technique::Annotation & aRequiredAnnotation)
                    {
                        const auto & availableAnnotations = technique.mAnnotations;
                        if(auto found = availableAnnotations.find(aRequiredAnnotation.mCategory);
                            found != availableAnnotations.end())
                        {
                            return (found->second == aRequiredAnnotation.mValue);
                        }
                        return false;
                    }))
            {
                return technique.mConfiguredProgram;
            }
        }

        return nullptr;
    }


    // TODO Ad 2023/10/05: for OpenGL resource, maybe we should directly use the GL name?
    // or do we want to reimplement Handle<> in term of index in storage containers?
    /// @brief Hackish class, associating a resource to an integer value.
    ///
    /// The value is suitable to compose a DrawEntry::Key via bitwise operations.
    template <class T_resource, unsigned int N_valueBits>
    struct ResourceIdMap
    {
        static_assert(N_valueBits <= 16);
        static constexpr std::uint16_t gMaxValue = (std::uint16_t)((1 << N_valueBits) - 1);

        ResourceIdMap()
        {
            // By adding the nullptr as the zero value, this helps debugging
            get(nullptr);
            assert(get(nullptr)== 0);
        }

        /// @brief Get the value associated to provided resource.
        /// A given map instance will always return the same value for the same resource.
        std::uint16_t get(Handle<T_resource> aResource)
        {
            assert(mResourceToId.size() < gMaxValue);
            auto [iterator, didInsert] = mResourceToId.try_emplace(aResource,
                                                                   (std::uint16_t)mResourceToId.size());
            return iterator->second;
        }

        /// @brief Return the resource from it value.
        Handle<T_resource> reverseLookup(std::uint16_t aValue)
        {
            if(auto found = std::find_if(mResourceToId.begin(), mResourceToId.end(),
                                         [aValue](const auto & aPair)
                                         {
                                             return aPair.second == aValue;
                                         });
               found != mResourceToId.end())
            {
                return found->first;
            }
            throw std::logic_error{"The provided value is not present in this ResourceIdMap."};
        }

        std::unordered_map<Handle<T_resource>, std::uint16_t> mResourceToId;
    };



} // unnamed namespace


/// @brief Specializes getProgram, returning first program matching provided pass name.
Handle<ConfiguredProgram> getProgramForPass(const Effect & aEffect, StringKey aPassName)
{
    const std::array<Technique::Annotation, 2> annotations{/*aggregate init of std::array*/{/*aggregate init of inner C-array*/
        {"pass", aPassName},
        {"pass", aPassName},
    }};
    return getProgram(aEffect, annotations.begin(), annotations.end());
}


/// @brief Returns the VAO matching the given program and part.
/// Under the hood, the VAO is cached.
Handle<graphics::VertexArrayObject> getVao(const ConfiguredProgram & aProgram,
                                           const Part & aPart,
                                           Storage & aStorage)
{
    // This is a common mistake, it would be nice to find a safer way
    assert(aProgram.mConfig);

    // Note: the config is via "handle", hosted by in cache that is mutable, so loosing the constness is correct.
    return
        [&, &entries = aProgram.mConfig->mEntries]() 
        -> graphics::VertexArrayObject *
        {
            if(auto foundConfig = std::find_if(entries.begin(), entries.end(),
                                            [&aPart](const auto & aEntry)
                                            {
                                                return aEntry.mVertexStream == aPart.mVertexStream;
                                            });
                foundConfig != entries.end())
            {
                return foundConfig->mVao;
            }
            else
            {
                aStorage.mVaos.push_back(prepareVAO(aProgram.mProgram, *aPart.mVertexStream));
                entries.push_back(
                    ProgramConfig::Entry{
                        .mVertexStream = aPart.mVertexStream,
                        .mVao = &aStorage.mVaos.back(),
                    });
                return entries.back().mVao;
            }
        }();
}


DrawEntryHelper::DrawEntryHelper() :
    mImpl{std::make_unique<Opaque>()}
{}


struct DrawEntryHelper::Opaque
{
    static constexpr unsigned int gProgramIdBits = 16;
    static constexpr std::uint64_t gProgramIdMask = makeMask(gProgramIdBits);

    static constexpr unsigned int gVaoBits = 10;
    static constexpr std::uint64_t gVaoIdMask = makeMask(gVaoBits);

    static constexpr unsigned int gMaterialContextBits = 10;
    static constexpr std::uint64_t gMaterialContextIdMask = makeMask(gMaterialContextBits);

    ResourceIdMap<ConfiguredProgram, gProgramIdBits> mProgramToId;
    ResourceIdMap<graphics::VertexArrayObject, gVaoBits> mVaoToId;
    ResourceIdMap<MaterialContext, gMaterialContextBits> mMaterialContextToId;

    std::vector<PartDrawEntry> generateDrawEntries(StringKey aPass,
                                                   const PartList & aPartList,
                                                   Storage & aStorage)
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "generate_draw_entries", CpuTime);

        using ProgramId = std::uint16_t;
        assert(aStorage.mPrograms.size() < (1 << gProgramIdBits));

        //constexpr unsigned int gPrimitiveModes = 12; // I counted 12 primitive modes
        //unsigned int gPrimitiveModeIdBits = (unsigned int)std::ceil(std::log2(gPrimitiveModes));

        using VaoId = std::uint16_t;
        assert(aStorage.mVaos.size() < (1 << gVaoBits));

        using MaterialContextId = std::uint16_t;
        assert(aStorage.mMaterialContexts.size() < (1 << gMaterialContextBits));

        //
        // For each part, associated it to its sort key and store them in an array
        // (this will already prune parts for which there is no program for aPass)
        //
        std::vector<PartDrawEntry> entries;
        entries.reserve(aPartList.mParts.size());

        for (std::size_t partIdx = 0; partIdx != aPartList.mParts.size(); ++partIdx)
        {
            const Part & part = *aPartList.mParts[partIdx];
            const Material & material = *aPartList.mMaterials[partIdx];

            // TODO Ad 2023/10/05: Also add those as sorting dimensions (to the LSB)
            assert(part.mPrimitiveMode == GL_TRIANGLES);
            assert(part.mVertexStream->mIndicesType == GL_UNSIGNED_INT);
            
            if(Handle<ConfiguredProgram> configuredProgram = getProgramForPass(*material.mEffect, aPass);
                configuredProgram)
            {
                Handle<graphics::VertexArrayObject> vao = getVao(*configuredProgram, part, aStorage);

                ProgramId programId = mProgramToId.get(configuredProgram);
                VaoId vaoId = mVaoToId.get(vao);
                MaterialContextId materialContextId = mMaterialContextToId.get(material.mContext);

                std::uint64_t key = vaoId ;
                key |= materialContextId << gVaoBits;
                key |= programId << (gVaoBits + gMaterialContextBits);

                entries.push_back(PartDrawEntry{.mKey = key, .mPartListIdx = partIdx});
            }
        }

        return entries;
    }

    DrawCall generateDrawCall(const PartDrawEntry & aEntry, const Part & aPart, const VertexStream & aVertexStream)
    {
        PartDrawEntry::Key drawKey = aEntry.mKey;
        return DrawCall{
            .mPrimitiveMode = aPart.mPrimitiveMode,
            .mIndicesType = aVertexStream.mIndicesType,
            .mProgram = 
                &mProgramToId.reverseLookup(gProgramIdMask & (drawKey >> (gVaoBits + gMaterialContextBits)))
                    ->mProgram,
            .mVao = mVaoToId.reverseLookup(gVaoIdMask & drawKey),
            .mCallContext = mMaterialContextToId.reverseLookup(gMaterialContextIdMask & (drawKey >> gVaoBits)),
            .mPartCount = 0,
        };
    }
};


std::vector<PartDrawEntry> DrawEntryHelper::generateDrawEntries(StringKey aPass,
                                                                const PartList & aPartList,
                                                                Storage & aStorage)
{
    return mImpl->generateDrawEntries(aPass, aPartList, aStorage);
}


DrawCall DrawEntryHelper::generateDrawCall(const PartDrawEntry & aEntry,
                                           const Part & aPart,
                                           const VertexStream & aVertexStream)
{
    return mImpl->generateDrawCall(aEntry, aPart, aVertexStream);
}


PassCache preparePass(StringKey aPass,
                      const PartList & aPartList,
                      Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "prepare_pass", CpuTime);

    DrawEntryHelper helper;
    std::vector<PartDrawEntry> entries = helper.generateDrawEntries(aPass, aPartList, aStorage);

    //
    // Sort the entries
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "sort_draw_entries", CpuTime);
        std::sort(entries.begin(), entries.end());
    }

    //
    // Traverse the sorted array to generate the actual draw commands and draw calls
    //
    PassCache result;
    PartDrawEntry::Key drawKey = PartDrawEntry::gInvalidKey;
    for(const PartDrawEntry & entry : entries)
    {
        const Part & part = *aPartList.mParts[entry.mPartListIdx];
        const VertexStream & vertexStream = *part.mVertexStream;

        // If true: this is the start of a new DrawCall
        if(entry.mKey != drawKey)
        {
            // Record the new drawkey
            drawKey = entry.mKey;
            // Push the new DrawCall
            result.mCalls.push_back(helper.generateDrawCall(entry, part, vertexStream));
        }
        
        // Increment the part count of current DrawCall
        ++result.mCalls.back().mPartCount;

        const std::size_t indiceSize = graphics::getByteSize(vertexStream.mIndicesType);

        // Indirect draw commands do not accept a (void*) offset, but a number of indices (firstIndex).
        // This means the offset into the buffer must be aligned with the index size.
        assert((vertexStream.mIndexBufferView.mOffset %  indiceSize) == 0);

        result.mDrawCommands.push_back(
            DrawElementsIndirectCommand{
                .mCount = part.mIndicesCount,
                // TODO Ad 2023/09/26: #bench #perf Is it worth it to group identical parts and do "instanced" drawing?
                // For the moment, draw a single instance for each part (i.e. no instancing)
                .mInstanceCount = 1,
                .mFirstIndex = (GLuint)(vertexStream.mIndexBufferView.mOffset / indiceSize)
                                + part.mIndexFirst,
                .mBaseVertex = part.mVertexFirst,
                .mBaseInstance = (GLuint)result.mDrawInstances.size(), // pushed below
            }
        );

        const Material & material = *aPartList.mMaterials[entry.mPartListIdx];

        result.mDrawInstances.push_back(DrawInstance{
            .mInstanceTransformIdx = aPartList.mTransformIdx[entry.mPartListIdx],
            .mMaterialIdx = (GLsizei)material.mPhongMaterialIdx,
            .mMatrixPaletteOffset = aPartList.mPaletteOffset[entry.mPartListIdx],
        });
    }

    return result;
}


} // namespace ad::renderer