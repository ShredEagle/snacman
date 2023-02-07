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

inline std::string keepLevels(std::string_view aStr, unsigned int aLevels = 1)
{
    auto pos = std::string::npos;
    const std::string delimiter = "::";
    for(; aLevels != 0U - 1U; --aLevels)
    {
        pos = aStr.rfind(delimiter, pos - 1);
        if (pos == std::string::npos)
        {
            return std::string{aStr};
        }
    }
    return std::string{aStr.substr(pos + delimiter.size())};
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

// The problem is that nvh profiler simply copies the pointer to the characters, not the string itself.
// As a workaround, keep a static string atm.
#define TIME_RECURRING_CLASSFUNC \
    static std::string keptStringForProfiling = ::ad::snac::keepLevels(__FUNCTION__, 1); \
    TIME_RECURRING(keptStringForProfiling.c_str())


} // namespace snac
} // namespace ad
