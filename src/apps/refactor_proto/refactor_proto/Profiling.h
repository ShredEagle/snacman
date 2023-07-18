#pragma once


#include "Profiler.h"


namespace ad::renderer {


inline Profiler & getGlobalProfiler()
{
    static Profiler globalProfiler;
    return globalProfiler;
}

// Note: Hardcoded for the moment, knowing in which order Providers are pushed into the Profiler.
enum 
{
    CpuTime = 0,
    GpuPrimitiveGen = 1,
};

#define PROFILER_BEGIN_FRAME ::ad::renderer::getGlobalProfiler().beginFrame()
#define PROFILER_END_FRAME ::ad::renderer::getGlobalProfiler().endFrame()

#define PROFILER_BEGIN_SECTION(name, ...) \
    auto profilerSectionUniqueName = ::ad::renderer::getGlobalProfiler().beginSection(name, {__VA_ARGS__})

#define PROFILER_END_SECTION ::ad::renderer::getGlobalProfiler().endSection(profilerSectionUniqueName)

#define PROFILER_SCOPE_SECTION(name, ...) \
    auto profilerScopedSection = ::ad::renderer::getGlobalProfiler().scopeSection(name, {__VA_ARGS__})

} // namespace ad::renderer