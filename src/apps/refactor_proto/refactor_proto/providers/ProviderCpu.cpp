#include "ProviderCpu.h"


namespace ad::renderer {


ProviderCPUTime::TimeInterval & ProviderCPUTime::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mTimePoints[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
}


void ProviderCPUTime::beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mBegin = Clock::now();
}


void ProviderCPUTime::endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mEnd = Clock::now();
}


bool ProviderCPUTime::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    // TODO address this case (potentially with Values of the correct type)
    aSampleResult = (GLuint)(interval.mEnd - interval.mBegin).count();
    //if(aSampleResult == 0)
    //{
    //    SELOG(warn)("Read a null sample, resolution of the timer might not be enough for some sections.");
    //}
    return true;
}


} // namespace ad::renderer