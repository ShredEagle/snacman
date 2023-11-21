#include "ProviderWindows.h"


#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <Windows.h>


namespace{


    inline std::int64_t getTicks()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceCounter(&tmp);
        return tmp.QuadPart;
    }


    inline std::int64_t getTickFrequency()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency(&tmp);
        return tmp.QuadPart;
    }


} // unnamed namespace


namespace ad::renderer {


ProviderCpuPerformanceCounter::ProviderCpuPerformanceCounter() : 
    ProviderInterface{
        "CPU (perfc)",
        "us",
        Ratio::MakeConversion(Ratio{1, (GLuint)getTickFrequency()}, Ratio::MakeRatio<std::micro>())}
{}


ProviderCpuPerformanceCounter::TimeInterval & ProviderCpuPerformanceCounter::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mTimePoints[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
}


void ProviderCpuPerformanceCounter::beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame) = -::getTicks();
}


void ProviderCpuPerformanceCounter::endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame) += ::getTicks();
}


bool ProviderCpuPerformanceCounter::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    // TODO address this case (potentially with Values of the correct type)
    aSampleResult = (GLuint)(interval);
    return true;
}


} // namespace ad::renderer