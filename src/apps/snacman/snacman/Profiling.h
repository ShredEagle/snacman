#pragma once

#include <snac-renderer/3rdparty/Profiler.hpp>

#include <handy/Guard.h>


namespace ad {
namespace snac {


inline nvh::Profiler & getProfiler()
{
    static nvh::Profiler gProfiler;
    return gProfiler;
}


inline Guard profileFrame()
{
    getProfiler().beginFrame();
    return Guard{[]{getProfiler().endFrame();}};
}

#define BEGIN_SINGLE(name, var) \
    auto var = std::make_optional(snac::getProfiler().timeSingle(name))

#define END_SINGLE(var) \
    var.reset()

#define TIME_SINGLE(name) \
    auto profilerSingleScoped = snac::getProfiler().timeSingle(name)

#define BEGIN_RECURRING(name, var) \
    auto var = std::make_optional(snac::getProfiler().timeRecurring(name))

#define END_RECURRING(var) \
    var.reset()

#define TIME_RECURRING(name) \
    auto profilerSectionScoped = snac::getProfiler().timeRecurring(name)

#define TIME_RECURRING_FUNC TIME_RECURRING(__func__)

#define TIME_RECURRING_FULLFUNC \
    TIME_RECURRING(__FUNCTION__)


} // namespace snac
} // namespace ad
