#pragma once

#include <profiler/Profiler.h>
#include <profiler/ProfilerRegistry.h>

namespace ad::renderer {


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

using Profiler_t = Profiler;


#define SCOPE_PROFILER(profiler) \
    ::ad::renderer::ProfilerRegistry::ScopeNewProfiler(profiler);
#define PROFILER_BEGIN_FRAME(profiler) ::ad::renderer::ProfilerRegistry::Get(profiler).beginFrame()
#define PROFILER_END_FRAME(profiler) ::ad::renderer::ProfilerRegistry::Get(profiler).endFrame()

#define PROFILER_PUSH_RECURRING_SECTION(profiler, name, ...) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_PUSH_SINGLESHOT_SECTION(profiler, entryIdx, name, ...) \
    auto entryIdx = ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

#define PROFILER_POP_RECURRING_SECTION(profiler) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).popCurrentSection()

#define PROFILER_POP_SECTION(profiler, entryIdx) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).endSection(entryIdx)

#define PROFILER_SCOPE_RECURRING_SECTION(profiler, name, ...) \
    auto profilerScopedSection = ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_SCOPE_SINGLESHOT_SECTION(profiler, name, ...) \
    auto profilerScopedSection = ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

#define PROFILER_PRINT_TO_STREAM(profiler, ostream) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).prettyPrint(ostream);

#else

#define SCOPE_RENDER_PROFILER(profiler, scopeName)

#define PROFILER_BEGIN_FRAME(profiler)
#define PROFILER_END_FRAME(profiler)

#define PROFILER_PUSH_RECURRING_SECTION(profiler, name, ...) 
#define PROFILER_PUSH_SINGLESHOT_SECTION(profiler, entryIdx, name, ...)
#define PROFILER_POP_RECURRING_SECTION(profiler)
#define PROFILER_POP_SECTION(profiler, entryIdx)

#define PROFILER_SCOPE_RECURRING_SECTION(profiler, name, ...)

#define PROFILER_SCOPE_SINGLESHOT_SECTION(profiler, name, ...)

#define PROFILER_PRINT_TO_STREAM(profiler, ostream)
#endif


inline Guard scopeProfilerFrame(const ProfilerRegistry::Key & aProfilerKey)
{
    PROFILER_BEGIN_FRAME(aProfilerKey);
    return Guard{[aProfilerKey]{PROFILER_END_FRAME(aProfilerKey);}};
}



} // namespace ad::renderer