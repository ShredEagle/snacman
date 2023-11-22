#include "ProviderRdtsc.h"

#include "../Logging.h"


namespace ad::renderer {


Ratio evluateRtscTickPeriodMicroSeconds()
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
    //assert(ticksPerMicrosecond < std::numeric_limits<Sample_t>::max());
    static_assert(std::is_same_v<ProviderInterface::Sample_t, std::uint64_t>);

    return Ratio{
        .num = 1,
        .den = ticksPerMicrosecond,
    };
}


} // namespace ad::renderer