#include "ProviderWindows.h"


#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <Windows.h>

#include <cassert>


namespace{


    inline std::int64_t getTicks()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceCounter(&tmp);
        return tmp.QuadPart;
    }


    inline std::uint64_t getTickFrequency()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency(&tmp);
        assert(tmp.QuadPart >= 0);
        return (std::uint64_t)tmp.QuadPart;
    }


} // unnamed namespace


namespace ad::renderer {


ProviderCpuPerformanceCounter::ProviderCpuPerformanceCounter() : 
    ProviderInterface{
        "CPU (perfc)",
        "us",
        Ratio::MakeConversion(Ratio{1, getTickFrequency()}, Ratio::MakeRatio<std::micro>())}
{}


ProviderCpuPerformanceCounter::TimeInterval & ProviderCpuPerformanceCounter::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mTimePoints[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
}


void ProviderCpuPerformanceCounter::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame) = -::getTicks();
}


void ProviderCpuPerformanceCounter::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame) += ::getTicks();
}


bool ProviderCpuPerformanceCounter::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, Sample_t & aSampleResult)
{
    aSampleResult = getInterval(aEntryIndex, aQueryFrame);
    return true;
}


} // namespace ad::renderer