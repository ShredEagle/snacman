#include "Pass.h"

#include "Profiling.h"
#include "SetupDrawing.h"

#include <profiler/GlApi.h>


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


Handle<ConfiguredProgram> getProgram(const Effect & aEffect, AnnotationsSelector aAnnotations)
{
    return getProgram(aEffect, aAnnotations.begin(), aAnnotations.end());
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


DrawEntryHelper::~DrawEntryHelper() = default;


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

    std::vector<PartDrawEntry> generateDrawEntries(AnnotationsSelector aAnnotations,
                                                   const PartList & aPartList,
                                                   Storage & aStorage)
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "generate_draw_entries", CpuTime);

        auto aParts = aPartList.mParts;
        auto aMaterials = aPartList.mMaterials;

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
        entries.reserve(aParts.size());

        for (std::size_t partIdx = 0; partIdx != aParts.size(); ++partIdx)
        {
            const Part & part = *aParts[partIdx];
            const Material & material = *aMaterials[partIdx];

            // TODO Ad 2023/10/05: Also add those as sorting dimensions (to the LSB)
            assert(part.mPrimitiveMode == GL_TRIANGLES);
            assert(part.mVertexStream->mIndicesType == GL_UNSIGNED_INT);
            
            if(Handle<ConfiguredProgram> configuredProgram = getProgram(*material.mEffect, aAnnotations);
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
            .mDrawCount = 0,
        };
    }
};


std::vector<PartDrawEntry> DrawEntryHelper::generateDrawEntries(AnnotationsSelector aAnnotations,
                                                                const PartList & aPartList,
                                                                Storage & aStorage)
{
    return mImpl->generateDrawEntries(aAnnotations, aPartList, aStorage);
}


DrawCall DrawEntryHelper::generateDrawCall(const PartDrawEntry & aEntry,
                                           const Part & aPart,
                                           const VertexStream & aVertexStream)
{
    return mImpl->generateDrawCall(aEntry, aPart, aVertexStream);
}


void draw(const PassCache & aPassCache,
          const RepositoryUbo & aUboRepository,
          const RepositoryTexture & aTextureRepository)
{
    //TODO Ad 2023/08/01: META todo, should we have "compiled state objects" (a-la VAO) for interface bocks, textures, etc
    // where we actually map a state setup (e.g. which texture name to which image unit and which sampler)
    // those could be "bound" before draw (potentially doing some binds and uniform setting, but not iterating the program)
    // (We could even separate actual texture from the "format", allowing to change an underlying texture without revisiting the program)
    // This would address the warnings repetitions (only issued when the compiled state is (re-)generated), and be better for perfs.

    GLuint firstInstance = 0; 
    for (const DrawCall & aCall : aPassCache.mCalls)
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "drawcall_iteration", CpuTime);

        const IntrospectProgram & selectedProgram = *aCall.mProgram;
        const graphics::VertexArrayObject & vao = *aCall.mVao;

        // TODO Ad 2023/10/05: #perf #azdo 
        // Only change what is necessary, instead of rebiding everything each time.
        // Since we sorted our draw calls, it is very likely that program remain the same, and VAO changes.
        {
            auto bindVaoProfiling = PROFILER_BEGIN_RECURRING_SECTION(gRenderProfiler, "bind_VAO", CpuTime);
            graphics::ScopedBind vaoScope{vao};
            PROFILER_END_SECTION(bindVaoProfiling);
            
            {
                PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_buffer_backed_blocks", CpuTime);
                // TODO #repos This should be consolidated
                RepositoryUbo uboRepo{aUboRepository};
                if(aCall.mCallContext)
                {
                    RepositoryUbo callRepo{aCall.mCallContext->mUboRepo};
                    uboRepo.merge(callRepo);
                }
                setBufferBackedBlocks(selectedProgram, uboRepo);
            }

            {
                PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_textures", CpuTime);
                // TODO #repos This should be consolidated
                RepositoryTexture textureRepo{aTextureRepository};
                if(aCall.mCallContext)
                {
                    RepositoryTexture callRepo{aCall.mCallContext->mTextureRepo};
                    textureRepo.merge(callRepo);
                }
                setTextures(selectedProgram, textureRepo);
            }

            auto bindProgramProfiling = PROFILER_BEGIN_RECURRING_SECTION(gRenderProfiler, "bind_program", CpuTime);
            graphics::ScopedBind programScope{selectedProgram};
            PROFILER_END_SECTION(bindProgramProfiling);

            {
                // TODO Ad 2023/08/23: Measuring GPU time here has a x2 impact on cpu performance
                // Can we have efficient GPU measures?
                PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "glDraw_call", CpuTime/*, GpuTime*/);
                
                gl.MultiDrawElementsIndirect(
                    aCall.mPrimitiveMode,
                    aCall.mIndicesType,
                    (void *)(firstInstance * sizeof(DrawElementsIndirectCommand)),
                    aCall.mDrawCount,
                    sizeof(DrawElementsIndirectCommand));
            }
        }
        firstInstance += aCall.mDrawCount;
    }
}


} // namespace ad::renderer