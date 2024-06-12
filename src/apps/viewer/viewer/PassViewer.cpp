#include "PassViewer.h"

#include <snac-renderer-V2/Profiling.h>


namespace ad::renderer {


ViewerPassCache prepareViewerPass(StringKey aPass,
                                  const ViewerPartList & aPartList,
                                  Storage & aStorage)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "prepare_pass", CpuTime);

    DrawEntryHelper helper;
    std::vector<PartDrawEntry> entries = 
        helper.generateDrawEntries(aPass,
                                   aPartList,
                                   aStorage);

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
    ViewerPassCache result;
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
        
        // Increment the draw count of current DrawCall:
        // Since we are drawing a single instance with each command (i.e. 1 DrawElementsIndirectCommand for each Part)
        // we increment the drawcount for each Part (so, with each PartDrawEntry)
        ++result.mCalls.back().mDrawCount;

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

        result.mDrawInstances.push_back(ViewerDrawInstance{
            .mInstanceTransformIdx = aPartList.mTransformIdx[entry.mPartListIdx],
            .mMaterialParametersIdx = (GLsizei)material.mMaterialParametersIdx,
            .mMatrixPaletteOffset = aPartList.mPaletteOffset[entry.mPartListIdx],
        });
    }

    return result;
}


} // namespace ad::renderer