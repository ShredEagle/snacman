#include "ProviderCpu.h"


namespace ad::renderer {


ProviderCPUTime::TimeInterval & ProviderCPUTime::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mTimePoints[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
}


void ProviderCPUTime::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mBegin = Clock::now();
}


void ProviderCPUTime::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mEnd = Clock::now();
}


bool ProviderCPUTime::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, Sample_t & aSampleResult)
{
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    // TODO address this case (potentially with Values of the correct type)
    aSampleResult = (Sample_t)(interval.mEnd - interval.mBegin).count();
    //if(aSampleResult == 0)
    //{
    //    SELOG(warn)("Read a null sample, resolution of the timer might not be enough for some sections.");
    //}
    return true;
}


} // namespace ad::renderer