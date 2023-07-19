#include "Profiling.h"


namespace ad::renderer {


std::unique_ptr<Profiler> globalProfiler;


Guard scopeGlobalProfiler()
{
    static std::once_flag initializeProfiler;
    bool passed = false;
    std::call_once(initializeProfiler, [&passed]()
    {
        globalProfiler = std::make_unique<Profiler>();
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