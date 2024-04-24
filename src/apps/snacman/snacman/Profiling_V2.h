#pragma once

#include <profiler/ProfilerRegistry.h>
#include <profiler/Profiling.h> 

#include <handy/Guard.h>

#include <array>
#include <mutex>


namespace ad {
namespace snac {
namespace v2 {


using Profiler_t = renderer::Profiler;
using ProfilerKey = renderer::ProfilerRegistry::Key;

// TODO implement a generic multi-buffering approach instead (to replace the critical section by atomic ptr switch)
// Generic should allow to reuse with ImGui.
class ProfilerPrint
{
public:
    ProfilerPrint(ProfilerKey aProfiler) :
        mProfiler{aProfiler}
    {}

    std::string get()
    {
        auto lock = std::scoped_lock{mStringMutex};
        return mPrinted;
    }

    // Should always be called from the same thread that does all the profiling on mProfiler.
    // (The thread safety in ensured on the string, not the Profiler)
    void print()
    {
        auto lock = std::scoped_lock{mStringMutex};
        std::ostringstream oss;
        PROFILER_PRINT_TO_STREAM(mProfiler, oss);
        mPrinted = oss.str();
    }

private:
    ProfilerKey mProfiler;
    std::string mPrinted;
    std::mutex mStringMutex;
};


inline ProfilerPrint & getRenderProfilerPrint()
{
    static ProfilerPrint profilerPrint{renderer::gRenderProfiler};
    return profilerPrint;
}


} // namespace v2
} // namespace snac
} // namespace ad
