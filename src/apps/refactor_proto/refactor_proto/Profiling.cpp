#include "Profiling.h"

#include <mutex>


namespace ad::renderer {


std::unique_ptr<Profiler_t> globalProfiler;


Guard scopeGlobalProfiler()
{
    static std::once_flag initializeProfiler;
    bool passed = false;
    std::call_once(initializeProfiler, [&passed]()
    {
        globalProfiler = std::make_unique<Profiler_t>();
        passed = true;
    });

    if (passed)
    {
        return Guard{[](){ globalProfiler.reset(); }};
    }
    else
    {
        return Guard{[](){}};
    }
}


} // namespace ad::renderer