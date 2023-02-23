#pragma once

#include <snac-renderer/3rdparty/nvpro/Profiler.hpp>

#include <handy/Guard.h>

#include <array>
#include <mutex>


namespace ad {
namespace snac {


enum class Profiler
{
    Main,
    Render,
};


inline nvh::Profiler & getProfiler(Profiler aProfiler)
{
    static std::array<nvh::Profiler, 2> gProfilers;
    return gProfilers[static_cast<std::size_t>(aProfiler)];
}


// TODO implement a generic multi-buffering approach instead (to replace the critical section by atomic ptr switch)
// Generic should allow to reuse with ImGui.
class ProfilerPrint
{
public:
    ProfilerPrint(Profiler aProfiler) :
        mProfiler{aProfiler}
    {}

    std::string get()
    {
        auto lock = std::scoped_lock{mStringMutex};
        return mPrinted;
    }

    void print()
    {
        auto lock = std::scoped_lock{mStringMutex};
        snac::getProfiler(mProfiler).print(mPrinted);
    }

private:
    Profiler mProfiler;
    std::string mPrinted;
    std::mutex mStringMutex;
};


inline ProfilerPrint & getRenderProfilerPrint()
{
    static ProfilerPrint profilerPrint{Profiler::Render};
    return profilerPrint;
}


inline Guard profileFrame(Profiler aProfiler)
{
    getProfiler(aProfiler).beginFrame();
    return Guard{[aProfiler]{getProfiler(aProfiler).endFrame();}};
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

inline std::string remove_signature(std::string_view aStr)
{
    auto pos = aStr.rfind('(', std::string::npos - 1);
    return std::string{aStr.substr(0, pos)};
}

#define BEGIN_SINGLE(profiler, name, var) \
    auto var = std::make_optional(snac::getProfiler(::ad::snac::Profiler::profiler).timeSingle(name))

#define END_SINGLE(var) \
    var.reset()

#define TIME_SINGLE(profiler, name) \
    auto profilerSingleScoped = snac::getProfiler(::ad::snac::Profiler::profiler).timeSingle(name)

#define BEGIN_RECURRING(profiler, name, var) \
    auto var = std::make_optional(snac::getProfiler(::ad::snac::Profiler::profiler).timeRecurring(name))

#define END_RECURRING(var) \
    var.reset()

#define TIME_RECURRING(profiler, name) \
    auto profilerSectionScoped = snac::getProfiler(::ad::snac::Profiler::profiler).timeRecurring(name)

#if defined(__GNUC__)
#define SNAC_FUNCTION_NAME ::ad::snac::remove_signature(__PRETTY_FUNCTION__)
#else
#define SNAC_FUNCTION_NAME __FUNCTION__
#endif

#define TIME_RECURRING_FUNC(profiler) TIME_RECURRING(profiler, __func__)

#define TIME_RECURRING_FULLFUNC(profiler) TIME_RECURRING(profiler, SNAC_FUNCTION_NAME)

// The problem is that nvh profiler simply copies the pointer to the characters, not the string itself.
// As a workaround, keep a static string atm.
#define TIME_RECURRING_CLASSFUNC(profiler) \
    static std::string keptStringForProfiling = ::ad::snac::keepLevels(SNAC_FUNCTION_NAME, 1); \
    TIME_RECURRING(profiler, keptStringForProfiling.c_str())


} // namespace snac
} // namespace ad
