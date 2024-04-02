#pragma once

#include "Profiling.h"

#include <snac-renderer-V1/3rdparty/nvpro/Profiler_gl.hpp>


namespace ad {
namespace snac {


inline nvgl::ProfilerGL & getProfilerGL()
{
    static nvgl::ProfilerGL gProfilerGL{&getProfiler(Profiler::Render)};
    return gProfilerGL;
}


#define TIME_RECURRING_GL(name) \
    auto profilerSectionScoped = ::ad::snac::getProfilerGL().timeRecurring(name); \
    PROFILER_SCOPE_RECURRING_SECTION(::ad::snac::ProfilerMap_V2::Render, name, ::ad::renderer::CpuTime, ::ad::renderer::GpuTime)


// TODO Ad 2024/04/02: #profilerv1 No need of the var anymore when decomissioning V1
// (the ID is returned to the caller)
#define BEGIN_RECURRING_GL(name, var) \
    PROFILER_PUSH_RECURRING_SECTION(::ad::snac::ProfilerMap_V2::Render, name, ::ad::renderer::CpuTime, ::ad::renderer::GpuTime); \
    auto var ## _gl = std::make_optional(::ad::snac::getProfilerGL().timeRecurring(name))
    
// TODO Ad 2024/04/02: #profilerv1 No need of the var anymore when decomissioning V1
#define END_RECURRING_GL(var, entryIdx) \
    var ## _gl.reset(); \
    PROFILER_POP_SECTION(::ad::snac::ProfilerMap_V2::Render, entryIdx)


#define TIME_RECURRING_CLASSFUNC_GL() \
    static std::string keptStringForProfiling = ::ad::snac::keepLevels(SNAC_FUNCTION_NAME, 1); \
    TIME_RECURRING_GL(keptStringForProfiling.c_str())


} // namespace snac
} // namespace ad
