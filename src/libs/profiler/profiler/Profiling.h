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


#define SCOPE_PROFILER(profiler, providerFeatures) \
    ::ad::renderer::ProfilerRegistry::ScopeNewProfiler(profiler, providerFeatures);
#define PROFILER_BEGIN_FRAME(profiler) ::ad::renderer::ProfilerRegistry::Get(profiler).beginFrame()
#define PROFILER_END_FRAME(profiler) ::ad::renderer::ProfilerRegistry::Get(profiler).endFrame()

//
// Scope unsafe macros
// With those, all exit points have to be matched with explicit "end scope" calls.
// I would say this is deprecated (unless at some point a real use for them emerges)
//

/// @deprecated
/// @note The returned EntryIdx can be captured, to pop the section explicitly with PROFILER_POP_SECTION.
/// Otherwise, the last section on the stack can be popped with PROFILER_POP_RECURRING_SECTION.
#define PROFILER_PUSH_UNSAFE_RECURRING_SECTION(profiler, name, ...) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

/// @deprecated
#define PROFILER_PUSH_UNSAFE_SINGLESHOT_SECTION(profiler, entryIdx, name, ...) \
    auto entryIdx = ::ad::renderer::ProfilerRegistry::Get(profiler).beginSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

/// @deprecated
/// @brief Pops the last section on the recurring sections stack.
#define PROFILER_POP_UNSAFE_RECURRING_SECTION(profiler) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).popCurrentSection()

/// @deprecated
#define PROFILER_POP_UNSAFE_SECTION(profiler, entryIdx) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).endSection(entryIdx)

//
// Scope-safe macros
// The used is intending to be able to terminate the sections before the end of the scope
// Yet the section would be automatically closed at scope exit
//
#define PROFILER_BEGIN_RECURRING_SECTION(profiler, name, ...) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::Recurring, name, {__VA_ARGS__})

#define PROFILER_BEGIN_SINGLESHOT_SECTION(profiler, entryIdx, name, ...) \
    ::ad::renderer::ProfilerRegistry::Get(profiler).scopeSection(::ad::renderer::EntryNature::SingleShot, name, {__VA_ARGS__})

#define PROFILER_END_SECTION(sectionGuard) \
    assert(sectionGuard.mProfiler != nullptr && "Likely double end-section."); \
    { auto expiringGuard{std::move(sectionGuard)}; } \
    /*request final ;*/assert(true)

// With this variant of the API, the user explicitly requests scoped lifetime of section
// so we do not (easily) return the guard to them.
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