#pragma once


#include "Profiler.h"


namespace ad::renderer {


inline Profiler & getGlobalProfiler()
{
    static Profiler globalProfiler;
    return globalProfiler;
}


#define PROFILER_BEGIN_FRAME ::ad::renderer::getGlobalProfiler().beginFrame()
#define PROFILER_END_FRAME ::ad::renderer::getGlobalProfiler().endFrame()

#define PROFILER_BEGIN_SECTION(name) auto profilerSectionUniqueName = ::ad::renderer::getGlobalProfiler().beginSection(name)
#define PROFILER_END_SECTION ::ad::renderer::getGlobalProfiler().endSection(profilerSectionUniqueName)


} // namespace ad::renderer