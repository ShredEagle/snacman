#pragma once


#include "Profiler.h"
#if defined(_WIN32)
#include "ProfilerSeattle.h"
#endif

#include <handy/Guard.h>


namespace ad::renderer {


using Profiler_t = Profiler;
//using Profiler_t = ProfilerSeattle;


extern std::unique_ptr<Profiler_t> globalProfiler;


// Because some providers within the profiler require to destory GL objects,
// we ask the client to explicitly scope the global profiler where the GL context is valid.
Guard scopeGlobalProfiler();


inline Profiler_t & getGlobalProfiler()
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
    DrawCalls,
    BufferMemoryWritten,
};

#ifdef SE_FEATURE_PROFILER

#define PROFILER_BEGIN_FRAME ::ad::renderer::getGlobalProfiler().beginFrame()
#define PROFILER_END_FRAME ::ad::renderer::getGlobalProfiler().endFrame()

#define PROFILER_PUSH_SECTION(name, ...) \
    ::ad::renderer::getGlobalProfiler().beginSection(name, {__VA_ARGS__})

#define PROFILER_POP_SECTION ::ad::renderer::getGlobalProfiler().popCurrentSection()

#define PROFILER_SCOPE_SECTION(name, ...) \
    auto profilerScopedSection = ::ad::renderer::getGlobalProfiler().scopeSection(name, {__VA_ARGS__})

#define PROFILER_PRINT_TO_STREAM(ostream) \
    ::ad::renderer::getGlobalProfiler().prettyPrint(ostream);

#else

#define PROFILER_BEGIN_FRAME
#define PROFILER_END_FRAME

#define PROFILER_PUSH_SECTION(name, ...) 
#define PROFILER_POP_SECTION

#define PROFILER_SCOPE_SECTION(name, ...)

#define PROFILER_PRINT_TO_STREAM(ostream)

#endif

} // namespace ad::renderer