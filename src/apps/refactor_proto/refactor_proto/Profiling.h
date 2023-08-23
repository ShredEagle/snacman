#pragma once


#include "Profiler.h"

#include <handy/Guard.h>


namespace ad::renderer {


extern std::unique_ptr<Profiler> globalProfiler;


// Because some providers within the profiler require to destory GL objects,
// we ask the client to explicitly scope the global profiler where the GL context is valid.
Guard scopeGlobalProfiler();


inline Profiler & getGlobalProfiler()
{
    return *globalProfiler;
}


// Note: Hardcoded list of available providers (for the time being),
// since we know in which order Providers are pushed into the Profiler.
enum 
{
    CpuTime = 0,
    GpuTime,
    GpuPrimitiveGen,
};

#ifdef SE_FEATURE_PROFILER

#define PROFILER_BEGIN_FRAME ::ad::renderer::getGlobalProfiler().beginFrame()
#define PROFILER_END_FRAME ::ad::renderer::getGlobalProfiler().endFrame()

#define PROFILER_BEGIN_SECTION(name, ...) \
    auto profilerSectionUniqueName = ::ad::renderer::getGlobalProfiler().beginSection(name, {__VA_ARGS__})

#define PROFILER_END_SECTION ::ad::renderer::getGlobalProfiler().endSection(profilerSectionUniqueName)

#define PROFILER_SCOPE_SECTION(name, ...) \
    auto profilerScopedSection = ::ad::renderer::getGlobalProfiler().scopeSection(name, {__VA_ARGS__})

#define PROFILER_PRINT_TO_STREAM(ostream) \
    ::ad::renderer::getGlobalProfiler().prettyPrint(ostream);

#else

#define PROFILER_BEGIN_FRAME
#define PROFILER_END_FRAME

#define PROFILER_BEGIN_SECTION(name, ...) 
#define PROFILER_END_SECTION

#define PROFILER_SCOPE_SECTION(name, ...)

#define PROFILER_PRINT_TO_STREAM(ostream)

#endif

} // namespace ad::renderer