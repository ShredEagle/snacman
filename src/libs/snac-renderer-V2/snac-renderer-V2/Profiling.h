#pragma once


#include <profiler/Profiler.h>
#if defined(_WIN32)
#include <profiler/ProfilerSeattle.h>
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

#define PROFILER_PUSH_RECURRING_SECTION(name, ...) \
    ::ad::renderer::getGlobalProfiler().beginSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_PUSH_SINGLESHOT_SECTION(entryIdx, name, ...) \
    auto entryIdx = ::ad::renderer::getGlobalProfiler().beginSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

#define PROFILER_POP_RECURRING_SECTION ::ad::renderer::getGlobalProfiler().popCurrentSection()

#define PROFILER_POP_SECTION(entryIdx) ::ad::renderer::getGlobalProfiler().endSection(entryIdx)

#define PROFILER_SCOPE_RECURRING_SECTION(name, ...) \
    auto profilerScopedSection = ::ad::renderer::getGlobalProfiler().scopeSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_SCOPE_SINGLESHOT_SECTION(name, ...) \
    auto profilerScopedSection = ::ad::renderer::getGlobalProfiler().scopeSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

#define PROFILER_PRINT_TO_STREAM(ostream) \
    ::ad::renderer::getGlobalProfiler().prettyPrint(ostream);

#else

#define PROFILER_BEGIN_FRAME
#define PROFILER_END_FRAME

#define PROFILER_PUSH_RECURRING_SECTION(name, ...) 
#define PROFILER_PUSH_SINGLESHOT_SECTION(entryIdx, name, ...)
#define PROFILER_POP_RECURRING_SECTION
#define PROFILER_POP_SECTION(entryIdx)

#define PROFILER_SCOPE_RECURRING_SECTION(name, ...)

#define PROFILER_SCOPE_SINGLESHOT_SECTION(name, ...)

#define PROFILER_PRINT_TO_STREAM(ostream)

#endif

} // namespace ad::renderer