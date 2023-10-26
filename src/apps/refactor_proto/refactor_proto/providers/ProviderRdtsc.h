#pragma once


#include "../Profiler.h"
#include "../Time.h"


namespace ad::renderer {


struct ProviderCpuRdtsc : public ProviderInterface
{
    // see https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter#examples
    using TickCount_t = std::uint64_t;

    ProviderCpuRdtsc();

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mIntervals.resize(aNewEntriesCount * Profiler::gFrameDelay); }

    ////// Logically private
    TickCount_t & getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<TickCount_t> mIntervals;
};


inline std::uint64_t readTsc()
{
    unsigned int ui;
    return __rdtscp(&ui);
}


Ratio evluateRtscTickPeriod()
{
    auto clockStart = Clock::now();
    auto tickCount = readTsc();

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    auto clockDuration = Clock::now() - clockStart;
    tickCount = readTsc() - tickCount;

    std::uint64_t ticksPerMicrosecond =
        tickCount / getTicks<std::chrono::microseconds>(clockDuration);
    SELOG(info)("TSC frequency evaluated to: {} MHz", ticksPerMicrosecond);
    // Check it does not overflow
    assert(ticksPerMicrosecond < std::numeric_limits<GLuint>::max());

    return Ratio{
        .num = 1,
        .den = (GLuint)(ticksPerMicrosecond),
    };
}

//
// Implementations
//

inline ProviderCpuRdtsc::ProviderCpuRdtsc():
        ProviderInterface{"CPU (tsc)", "us", evluateRtscTickPeriod()}
    {}

inline ProviderCpuRdtsc::TickCount_t & ProviderCpuRdtsc::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mIntervals[aEntryIndex * Profiler::gFrameDelay + aCurrentFrame];
}


inline void ProviderCpuRdtsc::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame) = readTsc();
}


inline void ProviderCpuRdtsc::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    auto & interval = getInterval(aEntryIndex, aCurrentFrame);
    interval = readTsc() - interval ;
}


bool ProviderCpuRdtsc::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    aSampleResult = (GLuint)(interval);
    return true;
}

} // namespace ad::renderer