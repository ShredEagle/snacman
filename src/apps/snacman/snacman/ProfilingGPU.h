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


#define TIME_RECURRING_GL(name, ...) \
    auto profilerSectionScoped = ::ad::snac::getProfilerGL().timeRecurring(name); \
    PROFILER_SCOPE_RECURRING_SECTION(::ad::snac::ProfilerMap_V2::Render, name, ::ad::renderer::CpuTime, ::ad::renderer::GpuTime, __VA_ARGS__)


#define BEGIN_RECURRING_GL(name, ...) \
    std::make_optional( \
        std::make_tuple( \
            ::ad::snac::getProfilerGL().timeRecurring(name), \
            ::ad::renderer::ProfilerRegistry::Get(::ad::snac::ProfilerMap_V2::Render) \
                .scopeSection(::ad::renderer::EntryNature::Recurring, name, { \
                    ::ad::renderer::CpuTime, \
                    ::ad::renderer::GpuTime, \
                    __VA_ARGS__}) \
        ) \
    )

    
#define END_RECURRING_GL(entry) \
    entry.reset()


#define TIME_RECURRING_CLASSFUNC_GL() \
    static std::string keptStringForProfiling = ::ad::snac::keepLevels(SNAC_FUNCTION_NAME, 1); \
    TIME_RECURRING_GL(keptStringForProfiling.c_str())


#define DISABLE_PROFILING_GL \
            ::ad::renderer::ProfilerRegistry::Get(::ad::snac::ProfilerMap_V2::Render) \
                .control(::ad::renderer::Profiler::SectionProfiling::Disabled)


#define ENABLE_PROFILING_GL \
            ::ad::renderer::ProfilerRegistry::Get(::ad::snac::ProfilerMap_V2::Render) \
                .control(::ad::renderer::Profiler::SectionProfiling::Enabled)


} // namespace snac
} // namespace ad
