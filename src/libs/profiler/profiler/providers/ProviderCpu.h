#pragma once


#include "../Profiler.h"

#include <chrono>
#include <cstddef>


namespace ad::renderer {


struct ProviderCPUTime : public ProviderInterface {
    using Clock = std::chrono::high_resolution_clock;

    ProviderCPUTime() : 
        ProviderInterface{"CPU time",
                          "us",
                           Ratio::MakeConversion<Clock::period, std::micro>()}
    {}

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, Sample_t & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mTimePoints.resize(aNewEntriesCount * Profiler::CountSubframes() ); }

    //////
    struct TimeInterval
    {
        Clock::time_point mBegin;
        Clock::time_point mEnd;
    };

    TimeInterval & getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<TimeInterval> mTimePoints;
};





} // namespace ad::renderer