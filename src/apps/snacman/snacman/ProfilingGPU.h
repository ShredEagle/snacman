#pragma once

#include "Profiling.h"

#include <snac-renderer/3rdparty/nvpro/Profiler_gl.hpp>


namespace ad {
namespace snac {


inline nvgl::ProfilerGL & getProfilerGL()
{
    static nvgl::ProfilerGL gProfilerGL{&getProfiler(Profiler::Render)};
    return gProfilerGL;
}


#define TIME_RECURRING_GL(name) \
    auto profilerSectionScoped = ::ad::snac::getProfilerGL().timeRecurring(name)


#define BEGIN_RECURRING_GL(name, var) \
    auto var ## _gl = std::make_optional(::ad::snac::getProfilerGL().timeRecurring(name))

#define END_RECURRING_GL(var) \
    var ## _gl.reset()

} // namespace snac
} // namespace ad
