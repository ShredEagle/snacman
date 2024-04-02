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

//// TODO Ad 2024/04/02: #profiling 
//// With this macro, all exit points have to be matched with explicit "end scope" calls.
/// It would be much better we have to RAII our way out of this.
//// This macro-based API should be reviewed, maybe only the scoped version should exist
//// (still offering an explicit macro function to end a scope early, taking the guard variable).
/// @note The returned EntryIdx can be captured, to pop the section explicitly with PROFILER_POP_SECTION.
/// Otherwise, the last section on the stack can be popped with PROFILER_POP_RECURRING_SECTION.
#define PROFILER_PUSH_RECURRING_SECTION(profiler, name, ...) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_PUSH_SINGLESHOT_SECTION(profiler, entryIdx, name, ...) \
    auto entryIdx = ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

/// @brief Pops the last section on the recurring sections stack.
#define PROFILER_POP_RECURRING_SECTION(profiler) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).popCurrentSection()

#define PROFILER_POP_SECTION(profiler, entryIdx) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).endSection(entryIdx)

// TODO Ad 2024/04/02: #profiling Should the variable be assigned directly by the client?
// (i.e. remove everything left-of and including the '=')
// Also, this might replace the PUSH_RECURRING, as long as POP_SECTION can take the scoping variable
#define PROFILER_SCOPE_RECURRING_SECTION(profiler, name, ...) \
    const auto profilerScopedSection_ ## __LINE__ = ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_SCOPE_SINGLESHOT_SECTION(profiler, name, ...) \
    const auto profilerScopedSection_ ## __LINE__ = ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

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