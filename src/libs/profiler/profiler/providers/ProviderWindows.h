#pragma once


#include "../Profiler.h"


namespace ad::renderer {


struct ProviderCpuPerformanceCounter : public ProviderInterface
{
    // see https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter#examples
    using TickCount_t = std::int64_t;

    ProviderCpuPerformanceCounter();

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, Sample_t & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mTimePoints.resize(aNewEntriesCount * Profiler::CountSubframes() ); }

    //////
    using TimeInterval = TickCount_t;
    TimeInterval & getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<TimeInterval> mTimePoints;
};


} // namespace ad::renderer