#include "EnvironmentMapping.h"

#include "../Cubemap.h"
#include "../Profiling.h"
#include "../Semantics.h"

#include "../files/Loader.h"

#include "../utilities/VertexStreamUtilities.h"


namespace ad::renderer {


Environment loadEnvironmentMap(const std::filesystem::path aDdsPath, Storage & aStorage)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "Load environment map",
                                      CpuTime, GpuTime, BufferMemoryWritten);

    return Environment{
        .mTextureRepository{ 
            {
                semantic::gEnvironmentTexture, 
                pushTexture(aStorage, loadCubemapFromDds(aDdsPath), "environment_map"),
            },
        },
    };
}



} // namespace ad::renderer